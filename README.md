# UI layout

## Implementation of Flutter's layout algorithm in C89

WzLayout does not care about the color of your widgets, or how you interact with it, the only thing it cares about is how to lay it out for you.
It's meant to get out of your way, and let you handle your ui which way you like. You pass it your choice of constraints and parameters for your widgets,
and it hands you a list of boxes representing the screen positions and width of your widgets. Plug in the boxes to your Ui,
throw some splash of color on them, some text, and voila you got yourself Ui. 
