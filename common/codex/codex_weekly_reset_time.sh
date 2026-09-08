#!/usr/bin/env bash
# Print the Codex weekly rate-limit reset time in this computer's local timezone.
#
# Requirements:
# - Codex CLI must be installed and logged in with ChatGPT.
# - Windows: run from Git Bash, MSYS2, Cygwin, or another Bash environment.
# - Linux: run from Bash. HTTPS_PROXY=http://127.0.0.1:7897 ./codex_weekly_reset_time.sh

set -euo pipefail

codex_command="${CODEX_COMMAND:-codex}"
timeout_seconds="${TIMEOUT_SECONDS:-45}"
json_output=0

usage() {
  cat <<'EOF'
Usage:
  ./codex-weekly-reset.sh [--json] [--timeout SECONDS] [--codex-command PATH]

Environment:
  CODEX_COMMAND     Optional Codex executable path or command name.
  TIMEOUT_SECONDS   Optional timeout in seconds. Default: 45.

Examples:
  ./codex-weekly-reset.sh
  ./codex-weekly-reset.sh --json
  ./codex-weekly-reset.sh --codex-command /path/to/codex
EOF
}

while (($# > 0)); do
  case "$1" in
    --json)
      json_output=1
      shift
      ;;
    --timeout)
      if (($# < 2)); then
        echo "Missing value for --timeout" >&2
        exit 2
      fi
      timeout_seconds="$2"
      shift 2
      ;;
    --codex-command)
      if (($# < 2)); then
        echo "Missing value for --codex-command" >&2
        exit 2
      fi
      codex_command="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

is_windows_bash() {
  case "$(uname -s 2>/dev/null || true)" in
    MINGW*|MSYS*|CYGWIN*) return 0 ;;
    *) return 1 ;;
  esac
}

to_unix_path() {
  local path_value="$1"
  if is_windows_bash && command -v cygpath >/dev/null 2>&1; then
    cygpath -u "$path_value"
  else
    printf '%s\n' "$path_value"
  fi
}

to_windows_path() {
  local path_value="$1"
  if is_windows_bash && command -v cygpath >/dev/null 2>&1; then
    cygpath -w "$path_value"
  else
    printf '%s\n' "$path_value"
  fi
}

get_user_home_unix() {
  if [[ -n "${HOME:-}" && -d "${HOME:-}" ]]; then
    to_unix_path "$HOME"
    return
  fi

  if [[ -n "${USERPROFILE:-}" ]]; then
    local user_profile_unix
    user_profile_unix="$(to_unix_path "$USERPROFILE")"
    if [[ -d "$user_profile_unix" ]]; then
      printf '%s\n' "$user_profile_unix"
      return
    fi
  fi

  echo "Could not determine the user home directory. Set HOME or USERPROFILE and try again." >&2
  exit 1
}

resolve_codex_executable() {
  local command_value="$1"
  local user_home_unix="$2"

  if [[ -f "$command_value" ]]; then
    printf '%s\n' "$command_value"
    return
  fi

  if command -v "$command_value" >/dev/null 2>&1; then
    command -v "$command_value"
    return
  fi

  local extension_root="$user_home_unix/.vscode/extensions"
  if [[ ! -d "$extension_root" ]]; then
    echo "Could not find '$command_value' in PATH, and no VS Code extension directory was found." >&2
    exit 1
  fi

  local candidates=()
  if is_windows_bash; then
    candidates=(
      "bin/windows-x86_64/codex.exe"
      "bin/windows-aarch64/codex.exe"
    )
  else
    candidates=(
      "bin/linux-x86_64/codex"
      "bin/linux-aarch64/codex"
    )
  fi

  shopt -s nullglob
  local extensions=("$extension_root"/openai.chatgpt-*)
  shopt -u nullglob

  local extension candidate relative_path
  for extension in "${extensions[@]}"; do
    for relative_path in "${candidates[@]}"; do
      candidate="$extension/$relative_path"
      if [[ -f "$candidate" ]]; then
        printf '%s\n' "$candidate"
        return
      fi
    done
  done

  echo "Could not find the Codex CLI. Install Codex CLI or pass --codex-command with the executable path." >&2
  exit 1
}

json_get_number() {
  local json_text="$1"
  local key="$2"
  sed -n "s/.*\"$key\":\([0-9][0-9]*\).*/\1/p" <<<"$json_text" | head -n 1
}

json_get_string() {
  local json_text="$1"
  local key="$2"
  sed -n "s/.*\"$key\":\"\([^\"]*\)\".*/\1/p" <<<"$json_text" | head -n 1
}

format_epoch_local() {
  local epoch_seconds="$1"
  if date -d "@$epoch_seconds" '+%Y-%m-%d %H:%M:%S %z' >/dev/null 2>&1; then
    date -d "@$epoch_seconds" '+%Y-%m-%d %H:%M:%S %z'
  elif date -r "$epoch_seconds" '+%Y-%m-%d %H:%M:%S %z' >/dev/null 2>&1; then
    date -r "$epoch_seconds" '+%Y-%m-%d %H:%M:%S %z'
  else
    echo "Could not format Unix timestamp with local date command." >&2
    exit 1
  fi
}

escape_json_string() {
  sed 's/\\/\\\\/g; s/"/\\"/g' <<<"$1"
}

read_codex_rate_limits() {
  local codex_path="$1"
  local user_home_unix="$2"
  local timeout="$3"

  local process_home="$user_home_unix"
  local process_codex_home="${CODEX_HOME:-$user_home_unix/.codex}"
  local process_home_drive=""
  local process_home_path=""

  if is_windows_bash; then
    process_home="$(to_windows_path "$user_home_unix")"
    if [[ -n "${CODEX_HOME:-}" ]]; then
      process_codex_home="$(to_windows_path "$CODEX_HOME")"
    else
      process_codex_home="$(to_windows_path "$user_home_unix/.codex")"
    fi

    if [[ "$process_home" =~ ^([A-Za-z]:)(\\.*)$ ]]; then
      process_home_drive="${BASH_REMATCH[1]}"
      process_home_path="${BASH_REMATCH[2]}"
    fi
  fi

  local stderr_file
  stderr_file="$(mktemp 2>/dev/null || mktemp -t codex-weekly-reset)"

  local init_request='{"id":1,"method":"initialize","params":{"clientInfo":{"name":"codex-weekly-reset-sh","version":"1.0.0"},"capabilities":{"experimentalApi":true}}}'
  local initialized_notification='{"method":"initialized"}'
  local rate_limits_request='{"id":2,"method":"account/rateLimits/read","params":null}'

  if is_windows_bash; then
    coproc CODEXPROC {
      env \
        HOME="$process_home" \
        USERPROFILE="$process_home" \
        HOMEDRIVE="$process_home_drive" \
        HOMEPATH="$process_home_path" \
        CODEX_HOME="$process_codex_home" \
        "$codex_path" app-server --stdio 2>"$stderr_file"
    }
  else
    coproc CODEXPROC {
      env \
        HOME="$process_home" \
        CODEX_HOME="$process_codex_home" \
        "$codex_path" app-server --stdio 2>"$stderr_file"
    }
  fi

  local line response deadline
  response=""
  deadline=$((SECONDS + timeout))

  printf '%s\n' "$init_request" >&"${CODEXPROC[1]}"
  printf '%s\n' "$initialized_notification" >&"${CODEXPROC[1]}"
  printf '%s\n' "$rate_limits_request" >&"${CODEXPROC[1]}"

  while ((SECONDS < deadline)); do
    if IFS= read -r -t 1 -u "${CODEXPROC[0]}" line; then
      if [[ "$line" == *'"id":2'* ]]; then
        response="$line"
        break
      fi
    elif ! kill -0 "$CODEXPROC_PID" >/dev/null 2>&1; then
      break
    fi
  done

  exec {CODEXPROC[1]}>&- 2>/dev/null || true
  kill "$CODEXPROC_PID" >/dev/null 2>&1 || true
  wait "$CODEXPROC_PID" >/dev/null 2>&1 || true

  if [[ -z "$response" ]]; then
    local stderr_text
    stderr_text="$(cat "$stderr_file" 2>/dev/null || true)"
    rm -f "$stderr_file"
    echo "Timed out waiting for Codex account/rateLimits/read response." >&2
    if [[ -n "$stderr_text" ]]; then
      echo "$stderr_text" >&2
    fi
    exit 1
  fi

  rm -f "$stderr_file"

  if [[ "$response" == *'"error":'* ]]; then
    local error_message
    error_message="$(json_get_string "$response" "message")"
    if [[ -z "$error_message" ]]; then
      error_message="$response"
    fi
    echo "Codex returned an error: $error_message" >&2
    exit 1
  fi

  printf '%s\n' "$response"
}

user_home_unix="$(get_user_home_unix)"
codex_path="$(resolve_codex_executable "$codex_command" "$user_home_unix")"
response_json="$(read_codex_rate_limits "$codex_path" "$user_home_unix" "$timeout_seconds")"

weekly_window="$(
  grep -Eo '"(primary|secondary)":\{[^{}]*\}' <<<"$response_json" |
    grep '"windowDurationMins":10080' |
    head -n 1 || true
)"

if [[ -z "$weekly_window" ]]; then
  weekly_window="$(
    grep -Eo '"(primary|secondary)":\{[^{}]*\}' <<<"$response_json" |
      sort -t ':' -k 1,1 |
      tail -n 1 || true
  )"
fi

if [[ -z "$weekly_window" ]]; then
  echo "Could not find a weekly rate-limit window in the Codex response." >&2
  exit 1
fi

window_name="$(sed -n 's/^"\([^"]*\)":.*/\1/p' <<<"$weekly_window" | head -n 1)"
reset_epoch_seconds="$(json_get_number "$weekly_window" "resetsAt")"
used_percent="$(json_get_number "$weekly_window" "usedPercent")"
window_duration_mins="$(json_get_number "$weekly_window" "windowDurationMins")"
plan_type="$(json_get_string "$response_json" "planType")"

if [[ -z "$reset_epoch_seconds" ]]; then
  echo "Could not read weekly resetsAt from the Codex response." >&2
  exit 1
fi

if [[ -z "$used_percent" ]]; then
  used_percent=0
fi

remaining_percent=$((100 - used_percent))
if ((remaining_percent < 0)); then
  remaining_percent=0
fi

reset_local="$(format_epoch_local "$reset_epoch_seconds")"
timezone_abbrev="$(date '+%Z' 2>/dev/null || true)"
timezone_offset="$(date '+%z' 2>/dev/null || true)"
timezone_abbrev="$(sed 's/^[[:space:]]*//; s/[[:space:]]*$//' <<<"$timezone_abbrev")"
timezone_offset="$(sed 's/^[[:space:]]*//; s/[[:space:]]*$//' <<<"$timezone_offset")"
if [[ -n "$timezone_abbrev" ]]; then
  timezone_display="$timezone_abbrev $timezone_offset"
else
  timezone_display="$timezone_offset"
fi

if ((json_output)); then
  reset_local_json="$(escape_json_string "$reset_local")"
  timezone_display_json="$(escape_json_string "$timezone_display")"
  codex_path_json="$(escape_json_string "$codex_path")"
  plan_type_json="$(escape_json_string "$plan_type")"
  cat <<EOF
{"weeklyResetLocal":"$reset_local_json","weeklyResetEpochSeconds":$reset_epoch_seconds,"timeZone":"$timezone_display_json","timeZoneOffset":"$timezone_offset","windowName":"$window_name","windowDurationMins":$window_duration_mins,"usedPercent":$used_percent,"remainingPercent":$remaining_percent,"planType":"$plan_type_json","codexExecutable":"$codex_path_json"}
EOF
else
  echo "Codex weekly reset (local time): $reset_local"
  echo "Local timezone: $timezone_display"
  echo "Weekly usage: ${used_percent}% used, ${remaining_percent}% remaining"
  echo "Window duration: ${window_duration_mins} minutes"
  if [[ -n "$plan_type" ]]; then
    echo "Plan: $plan_type"
  fi
fi
