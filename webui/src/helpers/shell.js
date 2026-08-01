/**
 * KernelSU / APatch / Magisk shell execution bridge
 * Isolated helper — all shell interactions flow through here
 */

let cbSeq = 0
let cachedInstalledApps = null

/**
 * Execute a shell command via the root manager bridge with 6s timeout protection
 * @param {string} cmd - Shell command to execute
 * @returns {Promise<string>} Command stdout
 */
export function execCommand(cmd) {
  return new Promise((resolve, reject) => {
    if (typeof ksu !== 'undefined' && typeof ksu.exec === 'function') {
      const id = `_hc_${++cbSeq}_${Date.now()}`
      
      const timer = setTimeout(() => {
        if (window[id]) {
          delete window[id]
          resolve('')
        }
      }, 6000)

      window[id] = (errno, stdout, stderr) => {
        clearTimeout(timer)
        delete window[id]
        resolve(stdout || stderr || '')
      }

      try {
        ksu.exec(cmd, '{}', id)
      } catch (e) {
        clearTimeout(timer)
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
 * List installed user (3rd-party) packages with in-memory caching
 * @param {boolean} forceRefresh
 * @returns {Promise<Array<{pkg: string, name: string}>>}
 */
export async function listInstalledApps(forceRefresh = false) {
  if (cachedInstalledApps && !forceRefresh) {
    return cachedInstalledApps
  }

  if (isKSU() && typeof ksu.listUserPackages === 'function') {
    try {
      const raw = ksu.listUserPackages()
      const pkgs = JSON.parse(raw)
      let infos = []
      if (typeof ksu.getPackagesInfo === 'function') {
        try { infos = JSON.parse(ksu.getPackagesInfo(raw)) } catch {}
      }
      cachedInstalledApps = pkgs.map((p, i) => ({
        pkg: p,
        name: (infos[i] && infos[i].appLabel) ? infos[i].appLabel : p
      })).sort((a, b) => a.name.localeCompare(b.name))

      return cachedInstalledApps
    } catch {}
  }

  // Fallback: pm list packages
  try {
    const out = await execCommand('pm list packages -3 2>/dev/null')
    cachedInstalledApps = out.trim().split('\n')
      .map(l => { const p = l.replace('package:', '').trim(); return { pkg: p, name: p } })
      .filter(i => i.pkg.length > 0)
      .sort((a, b) => a.pkg.localeCompare(b.pkg))

    return cachedInstalledApps
  } catch {
    return []
  }
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
