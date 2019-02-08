for file in ./*.ll
	do
		python simple-lit.py $file
	done