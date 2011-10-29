cd $1

KCM_FILES=$(dir *.kcm)

for file in $KCM_FILES
do
	./kcm $file $file.bin
	chmod 666 $file.bin
done
