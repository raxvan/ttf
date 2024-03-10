
def configure(cfg):
	cfg.disable()
	cfg.type("lib")

def construct(ctx):
	
	ctx.fscan("src:src")

	ctx.folder("public include:src")
	

