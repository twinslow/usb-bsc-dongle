Import("env")

def after_upload(source, target, env):
    #print("source=" + source + ", target=" + target + ", env=" + env )
    print("Delay while uploading...")
    import time
    time.sleep(3)
    print("Done!")

env.AddPostAction("upload", after_upload)