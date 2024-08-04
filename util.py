def good(text):
    print(f"[\033[95m+\033[0m] {text}")

def bad(text):
    print(f"[\033[31m-\033[0m] {text}")

def info(text):
    print(f"[\033[34m*\033[0m] {text}")

def print_ascii_art():
    # ANSI escape code for magenta (pink) color
    pink = '\033[95m'
    purple = '\033[34m'
    white = '\033[37m'
    # ANSI escape code to reset to default color
    reset = '\033[0m'

    print(f"""{purple}
  ________   _____   ___ ______ _____ ___   _____      ______  
 |___  /\\ \\ / /__ \\ / _ \\____  | ____|__ \\ / _ \\ \\    / /___ \\ 
    / /  \\ V /   ) | (_) |  / /| |__    ) | | | \\ \\  / /  __) |
   / /    > <   / / \\__, | / / |___ \\  / /| | | |\\ \\/ /  |__ < 
  / /__  / . \\ / /_   / / / /   ___) |/ /_| |_| | \\  /   ___) |
 /_____|/_/ \\_\\____| /_/ /_/   |____/|____|\\___/   \\/   |____/                                          
{reset}                    _    _          __   __
                   | |  | |   /\\    \\ \\ / /
                   | |__| |  /  \\    \\ V / 
                   |  __  | / /\\ \\    > <  
                   | |  | |/ ____ \\  / . \\ 
                   |_|  |_/_/    \\_\\/_/ \\_\\
    
          {reset}{purple}PoC's & Sploits for Kuwfi C920 (ZTE MF910W){reset}
""")