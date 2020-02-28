#!bin/bash

mknod /dev/secret c 20 0

chmod 666 /dev/secret

ls -l /dev/secret

service up /usr/src/drivers/secrets/secret -dev /dev/secret

cat /dev/secret

echo "The British are coming" > /dev/secret

echo "Another secret" > /dev/secret

cat /dev/secret

cat /dev/secret

echo "This secret is just for me" > /dev/secret

su victor

cat /dev/secret

cat > /dev/secret

exit

cat /dev/secret

su $1

echo "Its all mine now" > /dev/secret

exit

cat /dev/secret

su $1

cat /dev/secret

exit

ls -l test1.c

cat test1.c > /dev/secret

cat /dev/secret > a

diff a test1.c

ls -l  BigFile

cat BigFile > /dev/secret

cat /dev/secret > out

ls -la out

service down secret


































