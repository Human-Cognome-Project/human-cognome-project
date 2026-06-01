# Discord Relay — State & Architecture Notes

## What Was Built (2026-02-15)
All code in `/opt/project/repo/relay/` (gitignored — not core project)

### Components
- `core.py` — Multi-bot process manager, shared asyncio event loop + uvicorn API server
- `registry.py` — Bot registration with env var token refs + SHA-256 hash verification
- `auth.py` — Tiered trust (Tier 1=direct humans, Tier 2=API DIs, Tier 3=review gate)
- `router.py` — Message routing by channel/mention, review gate for Tier 3
- `api.py` — FastAPI REST + WebSocket for external connections (port 8400)
- `safety.py` — Injection screening (patterns, unicode, rate limiting)
- `monitor.py` — Terminal monitor: captures tmux pane deltas, posts to Discord channels
- `bootstrap.py` — First-run registration script

### Key Design Decisions
- Tokens NEVER stored in config — only env var names + SHA-256 hashes
- Hash verification on every startup prevents token hijacking
- One bot per specialist (not single bot with routing)
- Discord channels: topic channels (public, curated updates) + terminal channels (private, raw session deltas)
- Terminal channel access: specialist bot + Tier 1 users only (not other DIs — prevents context flooding)
- Monitor uses line-count checkpoint with compact detection (line count drop > 10 = compact)
- Delta splitting for Discord's 2000 char limit
- API for external teams (B's team NAS, future collaborators)

### Discord Server
- Guild ID: 1472657458018390198
- Patrick user ID: 1191049185009614890

### Channel IDs
- #general: 1472657458471239894
- #orchestrator: 1472673228651495457
- #db: 1472673231122071644
- #pbm: 1472673232862843022
- #librarian: 1472673234867716260
- #linguistics: 1472673236889370745
- #infra: 1472673239045116045
- #alerts: 1472673241188270341
- #review: 1472692365771935807
- #term-orchestrator: 1472698520162275511
- #term-db: 1472698525086253067
- #term-pbm: 1472698527758024768
- #term-librarian: 1472698531012808768
- #term-linguistics: 1472698535089541403
- #term-infra: 1472698539011473579
- #term-discord: 1472698542014337077

### Registered Bots
- chat_admin: HCP_CHAT_ADMIN#9817 (ID: 1472705899150839991), token env: DISCORD_TOKEN_HCP_CHAT_ADMIN
- bot: Human Cognome Project#5721 (ID: 1472665770721935390), token env: DISCORD_TOKEN_HCP_BOT

### Env Vars Set (in relay/.env)
- DISCORD_TOKEN_HCP_ORCHESTRATOR ✓
- DISCORD_TOKEN_HCP_DB ✓
- DISCORD_TOKEN_HCP_PBM ✓
- DISCORD_TOKEN_HCP_CHAT_ADMIN ✓
- DISCORD_TOKEN_HCP_BOT ✓
- DISCORD_TOKEN_HCP_LIB — placeholder
- DISCORD_TOKEN_HCP_LING — placeholder
- DISCORD_TOKEN_HCP_INFRA — placeholder

### Running Processes
- Relay (core.py): bots chat_admin + bot connected, API on port 8400
- Monitor (monitor.py): 15s interval, min 2-line delta, posting to term-* channels
- Monitor API key: hqAMNvnP_ZbkXcrKo2FTYSw87izcQJXjrbx62Nx51L8

### Remaining Tasks
- #8: MCP server interface for relay (so specialists use MCP tools instead of API)
- Terminal channel permissions (lock to bot + Tier 1 users via discord.py on_ready)
- Register remaining bots when Patrick creates tokens (orchestrator, db, pbm already have tokens)
- Wire monitor to post through each specialist's own bot (currently uses house bot)
- Patrick wants to discuss broader bot ecosystem vision
- Mattermost/Matrix evaluated as future alternatives if Discord outgrown

### Known Issues
- .mcp.json uses ${DISCORD_TOKEN_HCP_BOT} env var expansion — works with Claude Code
- "discord" blocked as bot name on Discord — using HCP_CHAT_ADMIN instead
- Monitor min-delta threshold: 1-line deltas from active sessions still slip through sometimes
- Terminal channels currently public — need permission lockdown
