#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cd $(dirname "$0")


cp src/include/* ./dist/include/zframe/

cp README.md ./dist/include/zframe/
cp LICENSE ./dist/include/zframe/

last_sha1=`git rev-parse HEAD`
last_date=`git show --pretty=format:"%ci" | head -1`
last_diff=`git log -1 --stat `

last_dist_sha1=`git log -1 --stat ./src |grep -E "commit ([0-9a-f]*)" |grep -E -o "[0-9a-f]{10,}"`
last_dist_date=`git show $last_dist_sha1 --pretty=format:"%ci" | head -1`
last_dist_diff=`git log -1 --stat ./src`

echo ""
echo "[zframe last commit]:"
echo $last_sha1
echo $last_date
echo ""
echo "[zframe last diff]:"
echo $last_diff


echo ""
echo "[./src last commit]:"
echo $last_dist_sha1
echo $last_dist_date
echo ""
echo "[./src last diff]:"
echo $last_dist_diff

echo ""
echo "[write versions]"
echo "version:" > ./dist/include/zframe/VERSION
echo "last_sha1(./src)=$last_dist_sha1" >> ./dist/include/zframe/VERSION 
echo "last_date(./src)=$last_dist_date" >> ./dist/include/zframe/VERSION 
echo "" >> ./dist/include/zframe/VERSION 
echo "git log -1 --stat ./src:" >> ./dist/include/zframe/VERSION 
echo $last_dist_diff >> ./dist/include/zframe/VERSION
cat ./dist/include/zframe/VERSION

echo ""
echo "[write done]"

