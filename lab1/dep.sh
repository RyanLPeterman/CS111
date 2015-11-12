echo anyorder1
echo anyorder2 | cat ; echo anyorder3; echo anyorder4
(echo anyorder5) && false || echo anyorder6

echo inorder1 > file.txt
cat < file.txt
echo inorder2 > file.txt
cat < file.txt

echo inorder3 > file.txt
cat < file.txt
echo inorder4 > file.txt
cat < file.txt

echo anyorder7 | cat ; echo anyorder8