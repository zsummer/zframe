#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cd $(dirname "$0")



echo $zbase_token

if [ "$zbase_token" != "" ]; then
	zbase_token=$zbase_token@
fi



git checkout 

if [ ! -d "./vender/zbase" ]; then
	git subtree add --prefix=vender/zbase --squash  https://${zbase_token}github.com/zsummer/zbase.git dist
fi

git subtree pull -q --prefix=vender/zbase --squash  https://${zbase_token}github.com/zsummer/zbase.git dist


if [ ! -d "./vender/fn-log" ]; then
	git subtree add --prefix=vender/fn-log --squash  https://github.com/zsummer/fn-log.git dist
fi

git subtree pull -q --prefix=vender/fn-log --squash  https://github.com/zsummer/fn-log.git dist

if [ ! -d "./vender/zprof" ]; then
	git subtree add --prefix=vender/zprof --squash  https://${zbase_token}github.com/zsummer/zprof.git dist
fi

git subtree pull -q --prefix=vender/zprof --squash  https://${zbase_token}github.com/zsummer/zprof.git dist








