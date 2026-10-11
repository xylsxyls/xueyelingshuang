// 仅给隔离副本添加定位类，不改变事件处理器或业务状态。
function instrumentWxml(source) {
  return source.replace(/<([\w:-]+)\b((?:[^>"']|"[^"]*"|'[^']*')*)>/g, (tag, name, attributes) => {
    const handlers = [...attributes.matchAll(/\b(?:bindtap|catchtap|bindchange|bindinput)="([A-Za-z_$][\w$]*)"/g)].map(match => 'cook-test-' + match[1])
    if (!handlers.length) return tag
    const markers = [...new Set(handlers)].join(' ')
    const selfClosing = /\/\s*$/.test(attributes)
    if (selfClosing) attributes = attributes.replace(/\/\s*$/, '')
    if (/\bclass="/.test(attributes)) attributes = attributes.replace(/\bclass="([^"]*)"/, (_, classes) => `class="${classes} ${markers}"`)
    else attributes += ` class="${markers}"`
    return `<${name}${attributes}${selfClosing ? ' /' : ''}>`
  })
}

function runtimeSelector(selector) {
  return selector.replace(/\[(?:bindtap|catchtap|bindchange|bindinput)="([A-Za-z_$][\w$]*)"\]/g, (_, handler) => '.cook-test-' + handler)
}

module.exports = { instrumentWxml, runtimeSelector }