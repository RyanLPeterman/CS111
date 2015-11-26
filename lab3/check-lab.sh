#! /bin/bash

function die() {
	echo "$@" >&2
	exit 1
}

[ $# = 1 ] || die "You must specify a lab directory."
cd "$1" || die "No such directory: $1"

[ -f answers.txt ] || die "No answers.txt file!"

BOLD="`echo -ne \\\\033[1m`"
NORM="`echo -ne \\\\033[0m`"

ids=`grep "^Student ID:" answers.txt | sed -e 's/^[^:]*: *//' | grep .`
[ "$ids" = "" ] && die "
*** You have not yet filled out the answers.txt file!
*** Fill out the answers.txt file and try again.
"

echo
echo "Here's the information in your answers.txt file."
echo "Please make sure it is correct."
echo

echo "Your name(s):$BOLD"
grep "^Name:" answers.txt | sed -e 's/^[^:]*:[ 	]*//' | grep . | sed -e 's/^/	/'
echo "$NORM"

echo "Your student ID(s):$BOLD"
grep "^Student ID:" answers.txt | sed -e 's/^[^:]*:[ 	]*//' | grep . | sed -e 's/^/	/'
echo "$NORM"

echo "Your email address(es):$BOLD"
grep "^Email:" answers.txt | sed -e 's/^[^:]*:[ 	]*//' | grep . | sed -e 's/^/	/'
echo "$NORM"

grep "^Extra credit problem:" answers.txt >/dev/null 2>&1 && EXTRA="yes; `grep "^Extra credit problem:" answers.txt | cut -d: -f 2- | sed -e 's/^[ ]*//'`" || EXTRA="no"
echo "Extra credit problem(s): $BOLD$EXTRA$NORM"
echo

echo -n "Is all this information correct? [y/N] "
read LINE
echo

[ "$LINE" == "y" -o "$LINE"  == "Y" ] || die "Please correct the answers.txt file and try again."

echo "*** Thank you. Please submit the file $BOLD$1.tar.gz$NORM
*** to only one CourseWeb account.
"
