#!/bin/sh

git filter-branch --env-filter '


n=$GIT_AUTHOR_NAME
m=$GIT_AUTHOR_EMAIL

case ${GIT_AUTHOR_NAME} in
    "(no author)") n="Ivan Kelly" ; m="ivan@ivankelly.net" ;;
    "ivan_kelly") n="Ivan Kelly" ; m="ivan@ivankelly.net" ;;
    "gagravarr") n="Nick Burch" ; m="gagravarr@users.sourceforge.net" ;;
esac

export GIT_AUTHOR_NAME="$n"
export GIT_AUTHOR_EMAIL="$m"
export GIT_COMMITTER_NAME="$n"
export GIT_COMMITTER_EMAIL="$m"
'

