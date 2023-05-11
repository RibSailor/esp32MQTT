Import ('env')

def after_upload(source, target, env):
    print ("Delay while uploading...")
    import time
    time.sleep(10)
    print ("Done!")

env.AddPostAction("upload", after_upload)