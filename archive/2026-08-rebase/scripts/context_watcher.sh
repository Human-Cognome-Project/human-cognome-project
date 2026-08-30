#!/usr/bin/env bash
# context_watcher.sh — Watches a Claude Code tmux pane for context reload signals
#
# Usage: ./context_watcher.sh <session_name>
#   e.g.: ./context_watcher.sh hcp_db
#
# Two-stage trigger:
#   1. Detects [CONTEXT_RELOAD_READY] → sends /clear
#   2. Detects post-clear header (▟█▙) → sends reload prompt
#
# Run one instance per specialist pane. Kill with Ctrl-C or `kill $PID`.
#
# Note: Uses C-m instead of Enter for submit — tmux Enter sometimes inserts
# a newline in Claude Code's input instead of submitting.

set -euo pipefail

SESSION="${1:?Usage: $0 <session_name>}"
POLL_INTERVAL=3
RELOAD_FILE="memory/${SESSION}_reload.md"

# States: watching | clearing | reloading | cooldown
STATE="watching"
COOLDOWN_UNTIL=0

log() {
    echo "[$(date '+%H:%M:%S')] [${SESSION}] $*"
}

send_and_submit() {
    # Send text then C-m to submit (Enter is unreliable in Claude Code via tmux)
    tmux send-keys -t "$SESSION" "$1"
    sleep 0.5
    tmux send-keys -t "$SESSION" C-m
}

capture_pane() {
    tmux capture-pane -t "$SESSION" -p 2>/dev/null || echo ""
}

log "Watching pane '${SESSION}' for context reload signals..."

while true; do
    NOW=$(date +%s)
    PANE_OUTPUT="$(capture_pane)"

    case "$STATE" in
        watching)
            # Skip if still in cooldown from last reload cycle
            if [ "$NOW" -lt "$COOLDOWN_UNTIL" ]; then
                sleep "$POLL_INTERVAL"
                continue
            fi
            # Only match the marker in agent output lines, not in user input (❯ prefix)
            if echo "$PANE_OUTPUT" | grep -v '❯' | grep -qF '[CONTEXT_RELOAD_READY]'; then
                log "Stage 1: Reload signal detected. Sending /clear..."
                sleep 2
                send_and_submit '/clear'
                STATE="clearing"
            fi
            ;;
        clearing)
            # Wait for the post-clear header art to appear
            if echo "$PANE_OUTPUT" | grep -q '▟█▙'; then
                log "Stage 2: Clear complete. Sending reload prompt..."
                sleep 2
                send_and_submit "Read your role file and ${RELOAD_FILE}, then continue working."
                STATE="reloading"
            fi
            ;;
        reloading)
            # Watch for the agent to start responding (● = tool use or output)
            if echo "$PANE_OUTPUT" | grep -q '●'; then
                log "Reload cycle complete. Cooldown 60s before watching again."
                # After a clear, the old marker is gone from pane buffer.
                # But add 60s cooldown as safety against any re-trigger.
                COOLDOWN_UNTIL=$(( NOW + 60 ))
                STATE="watching"
            fi
            ;;
    esac

    sleep "$POLL_INTERVAL"
done
