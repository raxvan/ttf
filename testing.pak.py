
def configure(cfg):
	cfg.disable()

def construct(ctx):
	
	ctx.config("type","lib")

	ctx.fscan("src:src")

	ctx.folder("public include:src")
	

