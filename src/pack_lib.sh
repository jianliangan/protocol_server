mkdir -p dist/lib
cmd=huage_rt
cp ./$cmd ./dist -a
cp ./config.json ./dist -a
deplist=$( ldd $cmd | awk '{if (match($3,"/")){ print $3}}' )
cp -L -n $deplist dist/lib
