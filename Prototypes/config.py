import argparse

visualize = False
save_audio = False
play_audio = False

# Initialize parser
parser = argparse.ArgumentParser()
parser.add_argument("-v", "--visualize", action='store_true', help = "Show Plots")
parser.add_argument("-s", "--save", action='store_true', help = "Save Audio")
parser.add_argument("-p", "--play", action='store_true', help = "Play Audio")

args = parser.parse_args()

if args.visualize:
    visualize = True
if args.save:
    save_audio = True
if args.play:
    play_audio = True

