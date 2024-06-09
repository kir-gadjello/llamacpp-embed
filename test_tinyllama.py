import os
import json

model = os.environ["HOME"] + "/projects/AI/tinyllama-1.1b-1t-openorca.Q4_K_M.gguf"

init_params_gpu = {
    "model": model,
    "n_gpu_layers": 99,
    "use_mmap": False,
    "ctx_size": 8192,
    "fa": True,
    "chat_template": "chatml"
}

init_params_cpu = {
    "model": model,
    "n_gpu_layers": 0,
    "use_mmap": False,
    "ctx_size": 8192,
    "fa": True,
    "chat_template": "chatml"
}

lib.init(json.dumps(init_params_cpu))
