#!/bin/sh
# Simple local importer: create .clurg and store a tarball snapshot as a 'commit'
set -e
repo_dir=$(pwd)
clurg_dir="$repo_dir/.clurg"
commits_dir="$clurg_dir/commits"

if [ ! -d "$clurg_dir" ]; then
  mkdir -p "$commits_dir"
  echo "initialized" > "$clurg_dir/REPO"
  echo "0" > "$commits_dir/HEAD"
fi

msg="$1"
if [ -z "$msg" ]; then
  msg="no message"
fi

id="$(date +%Y%m%d%H%M%S)-$$"
meta_file="$commits_dir/$id.meta"
archive_file="$commits_dir/$id.tar.gz"

# Ensure commits dir exists
mkdir -p "$commits_dir"

# create tarball excluding .clurg
tar --exclude='.clurg' -czf "$archive_file" .

echo "message: $msg" > "$meta_file"
echo "timestamp: $(date -Iseconds)" >> "$meta_file"
echo "id: $id" >> "$meta_file"
echo "cwd: $repo_dir" >> "$meta_file"

# Calculate SHA256 checksum of the archive
checksum=$(sha256sum "$archive_file" | cut -d' ' -f1)
echo "checksum: $checksum" >> "$meta_file"

# Create structured metadata.json
metadata_file="$commits_dir/$id.metadata.json"
ci_status="${CLURG_CI_STATUS:-unknown}"
cat > "$metadata_file" << EOF
{
  "id": "$id",
  "message": "$msg",
  "timestamp": "$(date -Iseconds)",
  "author": "$(whoami)",
  "cwd": "$repo_dir",
  "size_bytes": $(stat -c%s "$archive_file"),
  "checksum_sha256": "$checksum",
  "ci_status": "$ci_status",
  "files_count": $(tar -tf "$archive_file" | wc -l)
}
EOF

# update HEAD
echo "$id" > "$commits_dir/HEAD"

echo "Created local clurg commit: $id"

echo "meta: $meta_file"
echo "archive: $archive_file"

# Execute post-commit hook if it exists
hook_script="$repo_dir/.clurg/hooks/post-commit"
if [ -x "$hook_script" ]; then
    echo "Executing post-commit hook..."
    CLURG_COMMIT_ID="$id" CLURG_COMMIT_MESSAGE="$msg" CLURG_CI_STATUS="$ci_status" "$hook_script"
    if [ $? -eq 0 ]; then
        echo "Post-commit hook executed successfully"
    else
        echo "Warning: Post-commit hook failed"
    fi
else
    echo "No post-commit hook found (create executable script at .clurg/hooks/post-commit)"
fi

# Execute plugin hooks
if [ -f "$repo_dir/scripts/plugin-system.sh" ]; then
  # Usar bash explicitamente para suportar source
  # Executar suporte a plugins em background para não bloquear o commit
  ( bash -c "cd '$repo_dir' && source scripts/plugin-system.sh && run_hook 'post-commit' '$id' '$msg' '$ci_status'" >/dev/null 2>&1 & echo $! > "$commits_dir/plugin_last.pid" ) || true
fi
