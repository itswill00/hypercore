/**
 * KernelSU / APatch / Magisk shell execution bridge
 * Isolated helper — all shell interactions flow through here
 */

let cbSeq = 0

/**
 * Execute a shell command via the root manager bridge
 * @param {string} cmd - Shell command to execute
 * @returns {Promise<string>} Command stdout
 */
export function execCommand(cmd) {
  return new Promise((resolve, reject) => {
    if (typeof ksu !== 'undefined' && typeof ksu.exec === 'function') {
      const id = `_hc_${++cbSeq}_${Date.now()}`
      window[id] = (errno, stdout, stderr) => {
        delete window[id]
        resolve(stdout || stderr || '')
      }
      try {
        ksu.exec(cmd, '{}', id)
      } catch (e) {
        delete window[id]
        reject(e)
      }
    } else if (typeof exec === 'function') {
      exec(cmd)
        .then(r => resolve(typeof r === 'object' ? (r.stdout || r.stderr || '') : String(r)))
        .catch(reject)
    } else {
      reject(new Error('No root manager bridge available'))
    }
  })
}

/**
 * Sanitize a string for safe use in shell commands (package names, modes)
 * @param {string} s
 * @returns {string}
 */
export function sanitize(s) {
  return String(s).replace(/[^a-zA-Z0-9._:\-]/g, '').trim()
}

/**
 * Check if running inside KernelSU WebUI environment
 * @returns {boolean}
 */
export function isKSU() {
  return typeof ksu !== 'undefined'
}

/**
 * List installed user (3rd-party) packages
 * @returns {Promise<Array<{pkg: string, name: string}>>}
 */
export async function listInstalledApps() {
  if (isKSU() && typeof ksu.listUserPackages === 'function') {
    try {
      const raw = ksu.listUserPackages()
      const pkgs = JSON.parse(raw)
      let infos = []
      if (typeof ksu.getPackagesInfo === 'function') {
        try { infos = JSON.parse(ksu.getPackagesInfo(raw)) } catch {}
      }
      return pkgs.map((p, i) => ({
        pkg: p,
        name: (infos[i] && infos[i].appLabel) ? infos[i].appLabel : p
      })).sort((a, b) => a.name.localeCompare(b.name))
    } catch {}
  }

  // Fallback: pm list packages
  const out = await execCommand('pm list packages -3 2>/dev/null')
  return out.trim().split('\n')
    .map(l => { const p = l.replace('package:', '').trim(); return { pkg: p, name: p } })
    .filter(i => i.pkg.length > 0)
    .sort((a, b) => a.pkg.localeCompare(b.pkg))
}

/**
 * Get app icon URL (KernelSU scheme or empty)
 * @param {string} pkg
 * @returns {string}
 */
export function getIconUrl(pkg) {
  if (isKSU()) return `ksu://icon/${pkg}`
  return ''
}
