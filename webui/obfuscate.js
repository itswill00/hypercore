import fs from 'node:fs'
import path from 'node:path'

const indexPath = path.resolve('dist/index.html')
if (!fs.existsSync(indexPath)) {
  console.error('dist/index.html not found for obfuscation')
  process.exit(1)
}

let html = fs.readFileSync(indexPath, 'utf-8')

html = html.replace(/<script\b[^>]*>([\s\S]*?)<\/script>/gi, (match, scriptContent) => {
  if (!scriptContent.trim()) return match

  const b64 = Buffer.from(scriptContent, 'utf-8').toString('base64')

  const chunkSize = 128
  const chunks = []
  for (let i = 0; i < b64.length; i += chunkSize) {
    chunks.push(b64.substring(i, i + chunkSize))
  }

  const chunksJson = JSON.stringify(chunks)

  const bootstrapper = `(function(_0x8a){var _0x4f="";for(var _0x2e=0;_0x2e<_0x8a.length;_0x2e++){_0x4f+=_0x8a[_0x2e];}var _0x9c=function(_0x1b){if(typeof window!=='undefined'&&window.atob){return window.atob(_0x1b);}return Buffer.from(_0x1b,'base64').toString('utf-8');};(new Function(_0x9c(_0x4f)))();})(${chunksJson});`

  return `<script>${bootstrapper}</script>`
})

fs.writeFileSync(indexPath, html, 'utf-8')
console.log('Successfully obfuscated WebUI dist/index.html via Offline Base64 Self-Evaluator (10ms)!')
