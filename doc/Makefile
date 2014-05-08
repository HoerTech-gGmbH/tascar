IMG = filestructure.png fdp_render.pdf creator.png 

all: $(IMG)

clean:
	rm -f $(IMG)

%.png: %.dot
	dot -Tpng < $< > $@

%.pdf: %.dot
	dot -Tpdf < $< > $@

fdp_%.png: %.dot
	cat $< | sed 's/subgraph[^{]*//1' | fdp -Tpng > $@

fdp_%.pdf: %.dot
	cat $< | sed 's/subgraph[^{]*//1' | fdp -Tpdf > $@
