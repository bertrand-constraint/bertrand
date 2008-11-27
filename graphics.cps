cdef snews_setup()
	/painted false def
	/win framebuffer /new DefaultWindow send def
	{/FrameLabel (Bertrand Output) def
	 /PaintClient {/painted true store} def
	 /DestroyClient { (X\n) print } def
	 /ClientMenu [
		(ZAP) {(X\n) print currentprocess killprocessgroup}
	 ] /new DefaultMenu send def
	} win send
	/reshapefromuser win send
	/map win send
	{painted { exit } if pause} loop
	win /ClientCanvas get setcanvas
	/Screen findfont 14 scalefont setfont
cdef snews_line(x1,y1,x2,y2)
	x1 y1 moveto
	x2 y2 lineto
	stroke
cdef snews_string(string str,x,y)
	str stringwidth pop 2 div
	x exch sub y moveto str show
