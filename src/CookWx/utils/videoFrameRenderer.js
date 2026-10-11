function compileShader(gl, type, source) {
  const shader = gl.createShader(type)
  gl.shaderSource(shader, source)
  gl.compileShader(shader)
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    const message = gl.getShaderInfoLog(shader) || 'shader compile failed'
    gl.deleteShader(shader)
    throw new Error(message)
  }
  return shader
}

function createProgram(gl, fragmentSource) {
  const vertex = compileShader(gl, gl.VERTEX_SHADER,
    'attribute vec2 a_position; attribute vec2 a_texCoord; varying vec2 v_texCoord; void main(){ gl_Position=vec4(a_position,0.0,1.0); v_texCoord=a_texCoord; }')
  const fragment = compileShader(gl, gl.FRAGMENT_SHADER, fragmentSource)
  const program = gl.createProgram()
  gl.attachShader(program, vertex)
  gl.attachShader(program, fragment)
  gl.linkProgram(program)
  gl.deleteShader(vertex)
  gl.deleteShader(fragment)
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(program) || 'shader link failed')
  return program
}

function createTexture(gl) {
  const texture = gl.createTexture()
  gl.bindTexture(gl.TEXTURE_2D, texture)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
  return texture
}

class VideoFrameRenderer {
  constructor(canvas) {
    this.canvas = canvas
    this.gl = null
    this.context2d = null
    this.programs = null
    this.textures = null
    this.imageData = null
    this.supported = false
    this.initialize()
  }

  initialize() {
    try {
      const gl = this.canvas.getContext('webgl', { alpha: false, antialias: false, premultipliedAlpha: false })
      if (gl) {
        this.gl = gl
        this.programs = {
          i420: createProgram(gl,
            'precision mediump float; varying vec2 v_texCoord; uniform sampler2D u_y; uniform sampler2D u_u; uniform sampler2D u_v; void main(){ float y=1.1643*(texture2D(u_y,v_texCoord).r-0.0625); float u=texture2D(u_u,v_texCoord).r-0.5; float v=texture2D(u_v,v_texCoord).r-0.5; gl_FragColor=vec4(y+1.5958*v, y-0.39173*u-0.81290*v, y+2.017*u, 1.0); }'),
          rgba: createProgram(gl,
            'precision mediump float; varying vec2 v_texCoord; uniform sampler2D u_rgba; void main(){ gl_FragColor=texture2D(u_rgba,v_texCoord); }')
        }
        this.textures = [createTexture(gl), createTexture(gl), createTexture(gl)]
        this.vertices = gl.createBuffer()
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertices)
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
          -1, -1, 0, 1, 1, -1, 1, 1, -1, 1, 0, 0,
          -1, 1, 0, 0, 1, -1, 1, 1, 1, 1, 1, 0
        ]), gl.STATIC_DRAW)
        this.supported = true
        return
      }
    } catch (error) {
      this.gl = null
      this.programs = null
      this.textures = null
    }
    this.context2d = this.canvas.getContext('2d')
    this.supported = !!(this.gl || this.context2d)
  }

  resize(width, height) {
    if (!width || !height) return
    const scale = Math.min(1, 720 / Math.max(width, height))
    const outWidth = Math.max(2, Math.floor(width * scale / 2) * 2)
    const outHeight = Math.max(2, Math.floor(height * scale / 2) * 2)
    this.canvas.width = outWidth
    this.canvas.height = outHeight
    if (this.gl) this.gl.viewport(0, 0, outWidth, outHeight)
    if (this.context2d) this.imageData = this.context2d.createImageData(outWidth, outHeight)
  }

  frameBytes(frame) {
    if (!frame || !frame.data) return null
    if (frame.data instanceof ArrayBuffer) return new Uint8Array(frame.data)
    if (ArrayBuffer.isView(frame.data)) return new Uint8Array(frame.data.buffer, frame.data.byteOffset, frame.data.byteLength)
    return null
  }

  draw(frame) {
    const bytes = this.frameBytes(frame)
    if (!bytes || !frame.width || !frame.height) return false
    if (!this.canvas.width || !this.canvas.height) this.resize(frame.width, frame.height)
    const pixelCount = frame.width * frame.height
    if (bytes.byteLength === pixelCount * 4) return this.drawRgba(bytes, frame.width, frame.height)
    if (bytes.byteLength === pixelCount * 3 / 2) return this.drawI420(bytes, frame.width, frame.height)
    return false
  }

  drawRgba(bytes, width, height) {
    if (this.gl) {
      const gl = this.gl
      const program = this.programs.rgba
      gl.useProgram(program)
      this.bindVertices(program)
      gl.activeTexture(gl.TEXTURE0)
      gl.bindTexture(gl.TEXTURE_2D, this.textures[0])
      gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1)
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, bytes)
      gl.uniform1i(gl.getUniformLocation(program, 'u_rgba'), 0)
      gl.drawArrays(gl.TRIANGLES, 0, 6)
      return true
    }
    if (!this.context2d || width !== this.canvas.width || height !== this.canvas.height) return false
    if (!this.imageData) this.imageData = this.context2d.createImageData(width, height)
    this.imageData.data.set(bytes)
    this.context2d.putImageData(this.imageData, 0, 0)
    return true
  }

  drawI420(bytes, width, height) {
    if (!this.gl || width % 2 || height % 2) return false
    const gl = this.gl
    const ySize = width * height
    const chromaSize = ySize >> 2
    const planes = [bytes.subarray(0, ySize), bytes.subarray(ySize, ySize + chromaSize), bytes.subarray(ySize + chromaSize)]
    const sizes = [[width, height], [width >> 1, height >> 1], [width >> 1, height >> 1]]
    const program = this.programs.i420
    gl.useProgram(program)
    this.bindVertices(program)
    const names = ['u_y', 'u_u', 'u_v']
    for (let i = 0; i < 3; i += 1) {
      gl.activeTexture(gl.TEXTURE0 + i)
      gl.bindTexture(gl.TEXTURE_2D, this.textures[i])
      gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1)
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, sizes[i][0], sizes[i][1], 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, planes[i])
      gl.uniform1i(gl.getUniformLocation(program, names[i]), i)
    }
    gl.drawArrays(gl.TRIANGLES, 0, 6)
    return true
  }

  bindVertices(program) {
    const gl = this.gl
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vertices)
    const position = gl.getAttribLocation(program, 'a_position')
    const texCoord = gl.getAttribLocation(program, 'a_texCoord')
    gl.enableVertexAttribArray(position)
    gl.enableVertexAttribArray(texCoord)
    gl.vertexAttribPointer(position, 2, gl.FLOAT, false, 16, 0)
    gl.vertexAttribPointer(texCoord, 2, gl.FLOAT, false, 16, 8)
  }

  clear() {
    if (this.gl) {
      this.gl.clearColor(0, 0, 0, 1)
      this.gl.clear(this.gl.COLOR_BUFFER_BIT)
    } else if (this.context2d) {
      this.context2d.clearRect(0, 0, this.canvas.width, this.canvas.height)
    }
  }
}

module.exports = VideoFrameRenderer