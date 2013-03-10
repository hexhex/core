gsed -i.orig -e '/-Wl,-R/ { s/-Wl,-R/-Wl,-rpath,/; }' configure
./configure
