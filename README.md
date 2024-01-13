# Spirograph Maker

## Modes

### Editing Mode

This is the mode the program beings in.
You will be in editing mode to create and edit your spirograph.
Once your spirograph is created you can click `SPACE` to switch to _Animation mode_ and view your spirograph in action

Select a node:
Once you have nodes on the screen and in _Editing More_, you can always select a different node to edit. Simply click on the closet node to select it. A node will be highlighted when its in range of being selected next.

| Key         | Action                                                       |
| ----------- | ------------------------------------------------------------ |
| `E`         | Selected node's direction will follow the cursor             |
| `W`         | Selected node's position will follow the cursor              |
| `Q`         | Toggle the trail of the selected node                        |
| `R`         | Reset everything                                             |
| `BACKSPACE` | Delete's the selected node and all its children              |
| `SPACE`     | Switched over to _Animation Mode_ (click `R` to switch back) |
| `LCTRL`     | Enter _Create New Node Mode_                                 |

#### Create New Node Mode

Hold `LCTRL` (left control) to enter this mode. Drag your cursor to view and left click to add.

In this mode you can add a node wherever you like. This is where you will begin the program since you start by creating your first "root" node.

There will be an orange line connected to the closest available position to your cursor showing you what your new node will look like, left click to add the node.

You can hold to lock the new node in position and more accurately adjust the angle of your new node after you've chosen its position.

You can always add more child nodes to other nodes by holding `LCTRL` while in _Editing Mode_.

#### Colour Palette

While in _Editing Mode_, on the left hand side of the screen you will se many colours.
This palette controls the trail colour that the selected node will have when animated.

#### Speed Slider

While in _Editing Mode_, you can adjust the rotating speed of the selected node by sliding the slider located on the right hand side of the screen.

### Animation Mode

_Animation Mode_ will play the animation of your created spirograph.
Enter _Animation Mode_ by pressing `SPACE` when in _Editing Mode_.

| Key   | Action                                         |
| ----- | ---------------------------------------------- |
| SPACE | Pauses the animation                           |
| R     | End the animation and switch to _Editing Mode_ |
