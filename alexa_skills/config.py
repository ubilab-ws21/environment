CLUES_TABLE = {
        "globe": ["Find something to activate the room",
            "It's glowing in the dark, you should be able to see it"],
        "safe": ["Put the switches in the correct Position and divide the Input Voltage to the correct value for powering the safe",
            "Try to put the resistors in series and parallel to divide the voltage ",
            "Resistors in parallel decrease the resistance while resistors in series increase the overall resistance",
            "Try to use two one kilo ohm resistors and one two kilo ohm resistor",
            "Somewhere in this room must be a hint for remembering the safe code"],
        "door2": ["Laser can destroy or open things",
            "Light can be reflected",
            "Look in the mirror",
            "Redirect laser with mirrors",
            "One of the famous windows versions was released in this year",
            "The last number is 5",
            "Search for the biggest resistor",
            "Look for matching colors",
            "Color code indicates the right order",
            "Add up values",
            "Some numbers are binary",
            "0x9A",
            "The door can just be opened from the inside",
            "Push the button",
            "Use the lighted robot"],
        "retrieving_prototype":["Have you watched Indiana Jones? Maybe a famous scene inspires you.",
            "Can you see more floppy disks outside the safe?"],
        "prototype":["Try to replace the floppy disks inside the safe.",
            "Be patient, watch closely and reassemble the image",
            "You need all four floppy disks",
            "The floppy disk colors match the image colors"],
        "server": ["Look for the net map",
            "Follow the code"]
    }
ACTUAL_NAME_TO_PUZZLE = {
        "globe": "switching on the lights",
        "safe": "opening the safe",
        "door2": "opening the door",
        "retrieving_prototype": "retrieving the prototype",
        "prototype": "sending prototype data",
        "server": "shutting down stasis"
        }

class Puzzle:

    def __init__(self, puzzle_name, children=None):
        self.puzzle_name = puzzle_name
        self.children = children

    def get_children(self):
        return self.children

server = Puzzle("server")
prototype = Puzzle("prototype", children=[server])
door2 = Puzzle("door2", children=[prototype])
retrieving_prototype = Puzzle("retrieving_prototype")
safe = Puzzle("safe", children=[retrieving_prototype])
globe = Puzzle("globe", children=[safe, door2])
