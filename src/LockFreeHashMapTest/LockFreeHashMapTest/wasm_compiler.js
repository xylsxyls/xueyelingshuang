'use strict';

const fs = require('fs');
const path = require('path');
const child = require('child_process');

/** 通过 SDK 自带的驱动编译。Windows 批处理只接收固定的响应文件名，源码和输出路径不经命令解释器展开。 */
function compile(driver, args, options)
{
    if (process.platform !== 'win32') return child.spawnSync(driver, args, options);
    const list = args.map(arg => JSON.stringify(arg.replace(/\\/g, '/'))).join('\n');
    fs.writeFileSync(path.join(options.cwd, 'compile.rsp'), list, { flag: 'wx' });
    return child.spawnSync(process.env.ComSpec || 'cmd.exe',
        ['/d', '/v:off', '/s', '/c', '""%LFHM_COMPILER%" @compile.rsp"'],
        { ...options, windowsVerbatimArguments: true, env: { ...options.env, LFHM_COMPILER: driver } });
}

module.exports = compile;
