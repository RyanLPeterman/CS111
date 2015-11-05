echo anyorder
echo anyorder | cat ; echo anyorder; echo anyorder
(echo anyorder) && false || echo anyorder

echo inorder1 > file.txt
cat < file.txt
echo inorder2 > file.txt
cat < file.txt


echo inorder3 > file.txt
cat < file.txt
echo inorder4 > file.txt
cat < file.txt

echo anyorder | cat ; echo anyorder