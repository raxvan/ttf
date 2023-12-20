
def configure(cfg):
	cfg.disable()

def construct(ctx):
	
	ctx.setoption("type","lib")

	ctx.fscan("src:src")

	ctx.folder("public include:src")
	

