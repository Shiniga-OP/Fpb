#!/bin/sh
MSG="$1"
if [ -z "$MSG" ]; then
  echo "uso: sh git.sh \"mensagem\""
  exit 1
fi
git add . || exit 1
git commit -m "$MSG" || exit 1
git push origin HEAD || exit 1
