// 文件回收与实际占用独立于下载/播放器；磁盘操作异步执行。
function fileOperation(fs, method, options) {
  return new Promise((resolve, reject) => fs[method](Object.assign({}, options, { success: resolve, fail: reject })))
}

function managedKey(name) {
  return /^[0-9a-f]{2,16}_\d+_\d+\.mp4(?:\.part)?$/.test(name) ? name.replace(/\.mp4(?:\.part)?$/, '') : ''
}

class VideoCacheFiles {
  constructor(fs, directory) {
    this.fs = fs
    this.directory = directory
    this.files = Object.create(null)
    this.needsRefresh = true
    this.revision = 0
    this.pathRevisions = Object.create(null)
    this.deletingPaths = Object.create(null)
    this.queue = Promise.resolve()
  }

  schedule(action) {
    const result = this.queue.then(action)
    this.queue = result.then(() => {}, () => {})
    return result
  }

  refresh(index) { return this.schedule(() => this.readDirectory(index)) }

  async readDirectory(index) {
    this.needsRefresh = true
    const revision = this.revision
    const result = await fileOperation(this.fs, 'readdir', { dirPath: this.directory })
    const scanned = Object.create(null)
    const names = (result.files || []).filter(name => typeof name === 'string' && name !== '.' && name !== '..' && !/[\/\\]/.test(name))
    for (let offset = 0; offset < names.length; offset += 4) {
      await Promise.all(names.slice(offset, offset + 4).map(async name => {
        const path = this.directory + '/' + name
        try {
          const result = await fileOperation(this.fs, 'stat', { path, recursive: false })
          const stats = result.stats
          if (!stats || (typeof stats.isFile === 'function' && !stats.isFile())) return
          const bytes = Number(stats.size)
          if (!isFinite(bytes) || bytes < 0) throw new Error('无法读取视频缓存文件大小')
          scanned[path] = { path, key: managedKey(name), bytes }
        } catch (error) {
          if (!/ENOENT|no such file|not found/i.test(error && (error.errMsg || error.message) || '')) throw error
        }
      }))
    }
    // 扫描期间已提交或删除的文件以本次操作结果为准，不能被旧 stat 覆盖。
    Object.keys(this.pathRevisions).forEach(path => {
      if (this.pathRevisions[path] <= revision) return
      if (this.files[path]) scanned[path] = this.files[path]
      else delete scanned[path]
    })
    this.files = scanned
    this.pathRevisions = Object.create(null)
    this.needsRefresh = false
    Object.keys(index).forEach(key => {
      const entry = index[key]
      const expectedPath = this.directory + '/' + key + '.mp4'
      const actual = this.files[expectedPath]
      if (!entry || !actual || !actual.key || entry.path !== expectedPath || entry.key !== key) delete index[key]
      else entry.bytes = actual.bytes
    })
  }

  record(path, bytes) {
    if (!isFinite(bytes) || bytes < 0) { this.needsRefresh = true; return }
    if (!path.startsWith(this.directory + '/')) return
    const key = managedKey(path.slice(this.directory.length + 1))
    if (!key) return
    this.files[path] = { path, key, bytes }
    this.pathRevisions[path] = ++this.revision
  }

  forget(path) {
    delete this.files[path]
    this.pathRevisions[path] = ++this.revision
  }

  async remove(path, index) {
    const file = this.files && this.files[path]
    if (!file || !file.key || this.deletingPaths[path]) return false
    this.deletingPaths[path] = true
    try {
      await fileOperation(this.fs, 'unlink', { filePath: path })
      delete this.files[path]
      this.pathRevisions[path] = ++this.revision
      if (index[file.key] && index[file.key].path === path) delete index[file.key]
      return true
    } catch (error) {
      // ENOENT已无占用；其他删除错误保留文件和索引，不能假装已回收。
      if (/ENOENT|no such file|not found/i.test(error && (error.errMsg || error.message) || '')) {
        delete this.files[path]
        this.pathRevisions[path] = ++this.revision
        if (index[file.key] && index[file.key].path === path) delete index[file.key]
        return true
      }
      return false
    } finally { delete this.deletingPaths[path] }
  }

  trim(index, pins, pendingPaths, reservations, maxBytes, protectedPath) {
    return this.schedule(async () => {
      if (this.needsRefresh) await this.readDirectory(index)
      const reserved = () => Object.keys(reservations).reduce((sum, path) => sum + reservations[path], 0)
      const total = () => Object.keys(this.files).reduce((sum, path) => sum + (reservations[path] ? 0 : this.files[path].bytes), reserved())
      const entries = Object.keys(this.files).map(path => this.files[path]).sort((left, right) => {
        const access = file => index[file.key] && index[file.key].path === file.path ? Math.max(0, Number(index[file.key].lastAccess) || 0) : -1
        return access(left) - access(right)
      })
      let totalBytes = total()
      let removed = 0
      for (const file of entries) {
        if (!file.key || file.path === protectedPath || pins[file.path] || pendingPaths()[file.path] || reservations[file.path]) continue
        if (index[file.key] && index[file.key].path === file.path && totalBytes <= maxBytes) continue
        if (await this.remove(file.path, index)) { removed += 1; totalBytes = total() }
      }
      totalBytes = total()
      Object.keys(this.pathRevisions).forEach(path => { if (!this.files[path]) delete this.pathRevisions[path] })
      return { totalBytes, removed, fits: totalBytes <= maxBytes }
    })
  }
}

module.exports = VideoCacheFiles