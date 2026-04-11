import os

def print_dirs(directory, prefix="", is_top_level=True):
    entries = os.listdir(directory)
    entries = [e for e in entries if any(x in e for x in [".c", ".h"])]
    entries.sort(key=lambda e: (not os.path.isdir(os.path.join(directory, e)), e.lower()))

    for i, entry in enumerate(entries):
        full_path = os.path.join(directory, entry)
        is_last = (i == len(entries) - 1)

        if os.path.isdir(full_path):
            if is_top_level:
                print(f"{entry}/")
            else:
                connector = "└── " if is_last else "├── "
                print(prefix + connector + entry + "/")

            if is_top_level:
                new_prefix = "    " if is_last else "│   "
            else:
                new_prefix = prefix + ("    " if is_last else "│   ")

            # Recurse down into the directory
            print_dirs(full_path, new_prefix, is_top_level=False)

        else:
            if is_top_level:
                if not is_last:
                    print(f"│── {entry}")
                else:
                    print(f"└── {entry}")
            else:
                connector = "└── " if is_last else "├── "
                print(prefix + connector + entry)

if __name__ == '__main__':
    print_dirs('../WaffleOS/source')