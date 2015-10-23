#! /bin/sh

# UCLA CS 111 Lab 1 - Test that commands execute correctly

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
# simple command 
echo helloworld
# sequence command and subshell
(echo mynameis) && false || echo ryan
# pipe command
echo peterman | wc -m
# compound command
echo test ; echo doesitwork | cat ; echo hi
# ioredirection
echo gointoafile > test.txt
cat < test.txt
touch main.c
ls main.c && ls main.c
(echo done);(echo now)
EOF

cat >test.exp <<'EOF'
helloworld
mynameis
ryan
9
test
doesitwork
hi
gointoafile
main.c
main.c
done
now
EOF

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
