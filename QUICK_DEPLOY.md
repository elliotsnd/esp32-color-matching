# 🚀 ESP32 Color Matcher - Quick Deploy Reference

## TL;DR - Just Deploy My Changes!

```bash
npm run deploy
```

**That's it!** Your React changes are now live at `http://192.168.0.152`

---

## Common Commands

| Command | What it does |
|---------|-------------|
| `npm run deploy` | 🚀 **Full deployment** - Build → Copy → Upload to ESP32 |
| `npm run deploy:skip-upload` | 📦 **Build only** - Build → Copy (no ESP32 upload) |
| `npm run deploy:watch` | 👀 **Dev mode** - Auto-deploy when files change |
| `npm run build` | 🔨 **Build only** - Just build React app |

## Platform-Specific Quick Deploy

### Windows (Double-click)
```
scripts\deploy.bat
```

### PowerShell
```powershell
.\scripts\deploy.ps1
```

### Cross-platform (Make)
```bash
make deploy
```

---

## Development Workflow

### 1. Edit React Components
- Edit files in `components/`, `src/`, or root `.tsx` files
- Make your changes as usual

### 2. Deploy to ESP32
```bash
npm run deploy
```

### 3. View Results
- Open `http://192.168.0.152` in browser
- Press `Ctrl+F5` to refresh if needed

---

## Development Mode (Recommended)

```bash
npm run deploy:watch
```

- Automatically deploys when you save files
- Perfect for active development
- Press `Ctrl+C` to stop

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "PlatformIO not found" | `pip install platformio` |
| "Node.js not found" | Install from [nodejs.org](https://nodejs.org) |
| Changes not visible | Clear browser cache (`Ctrl+F5`) |
| ESP32 not responding | Check USB connection, try device reset |

---

## What Changed?

### ❌ Before (Manual Process)
1. `npm run build`
2. Copy `dist/*` to `data/` manually
3. `pio run --target uploadfs`
4. Wait and refresh browser

### ✅ After (Automated)
1. `npm run deploy`
2. ☕ Grab coffee while it deploys
3. Changes are live!

---

## File Structure (FYI)

```
📁 Your edits here:
├── components/          ← Edit React components here
├── src/                ← ESP32 firmware (C++)
├── *.tsx               ← Root React files

📁 Auto-generated (don't edit):
├── dist/               ← Vite build output
├── data/               ← ESP32 filesystem staging
└── scripts/            ← Deployment automation
```

---

## Need Help?

- 📖 **Full docs**: `docs/DEPLOYMENT_GUIDE.md`
- 🔍 **Verbose output**: `npm run deploy:verbose`
- 🆘 **Root cause analysis**: `docs/DEVELOPMENT_WORKFLOW.md`

---

**Happy coding! 🎉**
