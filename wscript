
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

import os.path

top = '.'
out = 'build'

def add_pngs(appinfo_filename, resources_dir):
    import sys
    import json
    import os.path
    import subprocess
    from os import listdir
    from os.path import isfile, join

    # load old dictionary if possible
    if os.path.lexists(appinfo_filename):
        appinfo_json = json.load(open(appinfo_filename, "rb"))

    #media_entries = appinfo_json['resources']['media']
    media_entries = []
    media_entries.append({
        "type": "raw", 
        "name": "ANIMATION_CONFIG", 
        "file": "animation_style_config.txt"
      })
    media_entries.append({
        "type": "png", 
        "name": "IMAGE_COVER_S4", 
        "file": "cover_s4.png"
      })

    pngs = [ f for f in listdir('resources') if isfile(join('resources',f)) ]
    pngs.sort()

    for png in pngs:
        if png.endswith('.png') and not png.startswith('cover'):
            media_entries.append(
              {"type": "png","name": "IMAGE_" + png.replace('-','_').replace('.png','').upper(), 
              "file": png})

    appinfo_json['resources']['media'] = media_entries

    # write the json dictionary
    json.dump(appinfo_json, open(appinfo_filename, "wb"), indent=2, sort_keys=False)
    return True

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    add_pngs('appinfo.json', 'resources')
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/**/*.js'))
