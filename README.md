# lue's ui modder ♡

A small Windows app for swapping Roblox UI textures (emotes, player list and cursor) with your own images. It works with both vanilla Roblox and Bloxstrap.

## Features

- **Two install types:** switch between **vanilla roblox** and **bloxstrap** with one click.
- **Emote wheel:** replace the emote wheel image with your own PNG.
- **Player list:** replace the player list avatar background with your own PNG.
- **Cursor:** replace the arrow cursors using a ZIP file containing your cursor images.
- **Mix and match:** pick any combination of the three and apply them all at once.
- **Remembers your files:** the files you choose are saved and loaded again the next time you open the app.
- **PNG check:** files are checked to make sure they are real PNGs, so a renamed JPG won't slip through.
- **Safe replacing:** files are swapped in one step, so a failed replace won't leave a half-written texture.
- **Clear feedback:** a message at the bottom of the window tells you what was replaced, or what went wrong.
- **Always up to date:** vanilla Roblox is detected automatically, so updates to Roblox don't break the app.
- **Tidy:** no backups are created and temporary files are cleaned up.

## How to use

1. **Pick your install type** at the top: **vanilla roblox** or **bloxstrap**.
2. **Click the name** of what you want to change (**emote**, **player list** or **cursor**) to open its upload panel.
3. In the panel, **click the upload box** and choose your file.
   - Emote and player list: a **PNG** image.
   - Cursor: a **ZIP** file containing `ArrowCursor.png` and `ArrowFarCursor.png`.
4. The file name appears in the box. Click **save** to close the panel, or **remove** to clear your choice.
5. **Tick the checkbox** next to every item you want to replace.
6. Click **replace texture**.
7. Read the message at the bottom of the window to confirm it worked.

Repeat for the other items as needed. Your chosen files stay selected between sessions, so next time you can just tick the boxes and press **replace texture**.

## Tips

- **Close Roblox first.** Files that are in use can't be replaced.
- **Emotes:** your one image is used for every emote wheel size.
- **Need the originals back?** Reinstall Roblox (or Bloxstrap's mods will be refreshed by reinstalling). The app doesn't keep backups.
- **Roblox updated?** Just press **replace texture** again, since updates can restore the original textures.

## Troubleshooting

| Message | What to do |
|---|---|
| *not found* | The selected install type isn't installed, or you haven't run it once yet. Check you picked the right one. |
| *choose a file first* | Click the item's name, upload a file, then try again. |
| *not a valid PNG* | The file isn't a real PNG. Export it again as PNG from your image editor. |
| *file in use* | Close Roblox completely and try again. |
| *not in zip* | Your cursor ZIP must contain `ArrowCursor.png` and `ArrowFarCursor.png`. |

## Requirements

- Windows 10 or 11
- Microsoft Edge WebView2 Runtime (already included with most Windows installs)

## Support

Questions or feedback? Join the Discord: **discord.gg/GmQ9HMy2ZE**