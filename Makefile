all: abt.cpp gbn.cpp sr.cpp
	g++ abt.cpp -o abt -g
	g++ gbn.cpp -o gbn -g
	g++ sr.cpp -o sr -g
	
clean: 
	rm -f abt sr gbn