# Fishing bot for WoW  

Uses boost, OpenCV, and Xdotools.  
Also includes a mouse mover method.  

## How to use  
Build the cmake project and run the executable in a terminal. To stop, press Ctrl+c.  

Required key combination inside the game:  
Key1: Fishing  
Key2: macro that applies bait, used every 10 minutes  
Key3: macro to open clams / chests, used every other catch  
"Auto loot" option must be enabled.  

## How it works
Takes screenshots of the game and evaluates them with OpenCV to find the blobber.  
Then moves the mouse to the blobber and monitors it closely, automatically right clicks if something was caught.


<img src="https://github.com/Lumajord/fishing_bot/blob/main/with_blobber.png" width="250"> - <img src="https://github.com/Lumajord/fishing_bot/blob/main/background.png" width="250"> = <img src="https://github.com/Lumajord/fishing_bot/blob/main/difference.png" width="250">  
