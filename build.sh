wget https://sqlite.org/2026/sqlite-amalgamation-3530400.zip
unzip sqlite-amalgamation-3530400.zip
rm sqlite-amalgamation-3530400.zip
mv sqlite-amalgamation-3530400 sqlite3

mkdir build && cd build
cmake .. && make
cp -r ../words ./
