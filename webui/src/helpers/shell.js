
let cbSeq = 0
let cachedInstalledApps = null

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

export function sanitize(s) {
  /* Strip characters outside a-z, A-Z, 0-9, ., _, :, - */
  return String(s).replace(/[^a-zA-Z0-9._:\-]/g, '').trim()
}

export function isKSU() {
  return typeof ksu !== 'undefined'
}

const KNOWN_APPS = {
  'com.mobile.legends': 'Mobile Legends: Bang Bang',
  'com.tencent.ig': 'PUBG Mobile',
  'com.pubg.krmobile': 'PUBG Mobile (KR)',
  'com.pubg.newstate': 'PUBG: New State',
  'com.vng.pubgmobile': 'PUBG Mobile (VNG)',
  'com.miHoYo.GenshinImpact': 'Genshin Impact',
  'com.HoYoverse.hkrpgoversea': 'Honkai: Star Rail',
  'com.miHoYo.ZenlessZoneZero': 'Zenless Zone Zero',
  'com.miHoYo.bh3global': 'Honkai Impact 3rd',
  'com.dts.freefireth': 'Free Fire',
  'com.dts.freefiremax': 'Free Fire MAX',
  'com.activision.callofduty.shooter': 'Call of Duty: Mobile',
  'com.mojang.minecraftpe': 'Minecraft',
  'com.roblox.client': 'Roblox',
  'com.supercell.clashofclans': 'Clash of Clans',
  'com.supercell.brawlstars': 'Brawl Stars',
  'com.supercell.clashroyale': 'Clash Royale',
  'com.supercell.squad': 'Squad Busters',
  'com.garena.game.kgid': 'Arena of Valor',
  'com.ea.gp.fifamobile': 'FC Mobile',
  'com.konami.efootball': 'eFootball 2024',
  'com.riotgames.league.wildrift': 'League of Legends: Wild Rift',
  'com.riotgames.league.teamfighttactics': 'TFT: Teamfight Tactics',
  'com.carxtech.sr': 'CarX Street',
  'com.carxtech.carxdr2': 'CarX Drift Racing 2',
  'com.innersloth.spacemafia': 'Among Us',
  'com.kiloo.subwaysurf': 'Subway Surfers',
  'com.outfit7.talkingtom': 'My Talking Tom',
  'com.kitkagames.fallbuddies': 'Stumble Guys',
  'com.levelinfinite.savage.wq': 'Undawn',
  'com.tencent.tmgp.pubgmhd': 'PUBG Mobile (CN)',
  'com.netease.newspike': 'NetEase NewSpike',
  'nu.gpu.nagramx': 'NagramX',
  'moe.rukamori.archivetune': 'ArchiveTune',
  'com.reddit.frontpage': 'Reddit',
  'com.quora.android': 'Quora',
  'com.pinterest': 'Pinterest',
  'com.axis.net': 'Axis Net',
  'com.miui.calculator': 'Calculator'
}

export function formatPkgName(pkg) {
  if (!pkg) return ''
  if (KNOWN_APPS[pkg]) return KNOWN_APPS[pkg]

  const parts = pkg.split('.').filter(Boolean)
  if (parts.length === 0) return pkg

  let last = parts[parts.length - 1]
  if (['android', 'app', 'client', 'game', 'gp'].includes(last.toLowerCase())) {
    if (parts.length > 1) last = parts[parts.length - 2]
  }

  const formatted = last
    .replace(/([a-z])([A-Z])/g, '$1 $2')
    .replace(/[_\-.]+/g, ' ')
    .replace(/\b\w/g, c => c.toUpperCase())
    .trim()

  return formatted || pkg
}

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
      const infoMap = {}
      const iconMap = {}
      if (Array.isArray(infos)) {
        infos.forEach(info => {
          if (info && info.packageName) {
            infoMap[info.packageName] = info.appLabel || formatPkgName(info.packageName)
            if (info.icon || info.appIcon) {
              iconMap[info.packageName] = info.icon || info.appIcon
            }
          }
        })
      }
      cachedInstalledApps = pkgs.map(p => ({
        pkg: p,
        name: infoMap[p] || formatPkgName(p),
        icon: iconMap[p] || getIconUrl(p)
      })).sort((a, b) => a.name.localeCompare(b.name))

      return cachedInstalledApps
    } catch {}
  }

  try {
    const out = await execCommand('pm list packages -3 2>/dev/null')
    cachedInstalledApps = out.trim().split('\n')
      .map(l => {
        const p = l.replace('package:', '').trim()
        return { pkg: p, name: formatPkgName(p), icon: getIconUrl(p) }
      })
      .filter(i => i.pkg.length > 0)
      .sort((a, b) => a.name.localeCompare(b.name))

    return cachedInstalledApps
  } catch {
    return []
  }
}

export function getIconUrl(pkg) {
  if (!pkg) return ''
  if (isKSU()) {
    if (typeof ksu.getAppIcon === 'function') {
      try {
        const icon = ksu.getAppIcon(pkg)
        if (icon) return icon
      } catch {}
    }
    if (typeof ksu.getIcon === 'function') {
      try {
        const icon = ksu.getIcon(pkg)
        if (icon) return icon
      } catch {}
    }
    return `ksu://icon/${pkg}`
  }
  return ''
}

export async function openExternal(url) {
  if (!url) return
  const cleanUrl = String(url).trim()

  try {
    if (typeof ksu !== 'undefined' && typeof ksu.open === 'function') {
      ksu.open(cleanUrl)
      return
    }
  } catch (e) {}

  try {
    const res = await execCommand(`am start -a android.intent.action.VIEW -d "${cleanUrl}" 2>&1`)
    if (res && res.includes('Starting: Intent')) {
      return
    }
  } catch (e) {}

  try {
    const a = document.createElement('a')
    a.href = cleanUrl
    a.target = '_blank'
    a.rel = 'noopener noreferrer'
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
  } catch (e) {}
}
