# Dialog windows

## Alias resolutions dialog

This dialog can be accessed with <menu-path>Context > Input > Aliases...</menu-path>.

With the alias resolutions dialog, you can instruct VCS to automatically override certain capture resolutions.

For instance, if you find that your capture device is displaying 640 &times; 400 as 512 &times; 488 (or something to that effect), you can define 640 &times; 400 as an alias of 512 &times; 488. Whenever the capture device reports a new video mode of 512 &times; 488, VCS will tell the device to use 640 &times; 400, instead.

## Anti-tear dialog

This dialog can be accessed with <key-combo>Ctrl + A</key-combo> or <menu-path>Context > Output > Anti-tear...</menu-path>.

> A screenshot of the anti-tear dialog\
![{image:489x502}{headerless}](../img/anti-tear-dialog.webp)

The anti-tear dialog provides functionality to remove tearing from captured frames.

Under some circumstances, like when the captured source doesn't sync its rendering with the refresh rate, captured frames can contain tearing. VCS's anti-tearer helps mitigate this issue.

Anti-tearing should be considered an experimental feature of VCS. It works well in some cases and not that well in others. It'll completely fail to work if the captured source redraws the screen at a rate higher than the capture's refresh rate &ndash; e.g. a game running at 100 FPS with a refresh of 60 Hz.

### Settings explained

<dokki-table headerless>
    <table>
        <tr>
            <th>Setting</th>
            <th>Description</th>
        </tr>
        <tr>
            <td>Scan start</td>
            <td>
                Set where the anti-tearer begins scanning each frame for tears. Static screen-wide content like a game's UI bar can prevent the anti-tearing from working, so you should set this value so that such content is excluded. You can choose to visualize the scan range to help you set it up.
            </td>
        </tr>
        <tr>
            <td>Scan end</td>
            <td>
                Same as <strong>Scan start</strong> but for where the scanning should end. This is an offset from the bottom of the screen up, so e.g. a value of 5 at a resolution of 640 &times; 480 would mean the scanning ends at pixel row 475.
            </td>
        </tr>
        <tr>
            <td>Scan direction</td>
            <td>
                If the captured source redraws its screen from bottom to top, set the scan direction to <em>Down</em>. Otherwise, use the <em>Up</em> setting. Using the wrong direction will fully prevent the anti-tearing from working (it may correctly detect tears but won't be able to remove them).
            </td>
        </tr>
        <tr>
            <td>Scan hint</td>
            <td>
                If the captured source redraws its screen at a rate higher than half of its refresh rate but lower than the full refresh rate (e.g. 35 FPS at 60 Hz), you may (or might not) have better results and/or performance by setting this option to <em>Look for one tear per frame</em>. Otherwise, use the <em>Look for multiple tears per frame</em> setting.
            </td>
        </tr>
        <tr>
            <td>Visualization</td>
            <td>
                Draw certain anti-tearing-related markers in the <a href="#output-window">output window</a>.
            </td>
        </tr>
        <tr>
            <td>Threshold</td>
            <td>
                The anti-tearer compares adjacent frames to find which parts of the new frame may be torn (where pixels from the previous frame are still visible). This setting controls the amount by which a pixel's color values are allowed to change between frames without the pixel being considered new (given inherent noise in analog pixels). In an ideal situation where there's no noise in the captured signal, you can set this to 0 or close to it. Otherwise, the value should be high enough to exclude capture noise.
            </td>
        </tr>
        <tr>
            <td>Window size</td>
            <td>
                When scanning frames for tears, the anti-tearer will average together a bunch of horizontal pixels' color values to reduce the negative effect of analog noise. This setting controls the pixel size of the sampling window. Lower values will result in better performance but possibly worse tear detection.
            </td>
        </tr>
        <tr>
            <td>Step size</td>
            <td>
                The number of pixels to skip horizontally when scanning for tears. Higher values will improve performance but may cause a failure to detect subtler tears.
            </td>
        </tr>
        <tr>
            <td>Matches req'd</td>
            <td>
                Set how many times the sampling window must find a pixel's color values to have exceeded the detection threshold for a horizontal row of pixels to be considered new relative to the previous frame. Higher values should reduce the chance of false positives but may also cause a failure to detect subtler tears.
            </td>
        </tr>
    </table>
</dokki-table>

## Filter graph dialog

This dialog can be accessed with <key-combo>Ctrl + F</key-combo> or <menu-path>Context > Output > Filter graph...</menu-path>.

> The filter graph dialog showing a chain of frame filters\
![{image}{headerless}](../img/vcs-2.6-filter-graph.webp)

The filter graph dialog lets you to create chains of image filters to be applied to captured frames prior to display in the [output window](#output-window).

The filter graph is made up of nodes that can be connected together in a chain. These nodes come in three varieties: *input gate*, *output gate*, and *filter*.

The input and output gates determine the resolutions for which the connected filters will be applied. For instance, if you set an input gate's width and height to 640 and 480, and the width and height of an output gate to 1920 and 1080, any filters you connect between these two nodes will be applied when the size of the output window is 1920 &times; 1080 and the original resolution of the frames (i.e. the capture resolution) is 640 &times; 480. You can also use the value 0 for a gate's width and/or height to allow VCS to match any value to that dimension: an input gate with a width and height of 0, for instance, will apply the connected filters to frames of all capture resolutions, provided that they also meet the resolution specified for the output gate. A filter graph can have multiple chains of these input-filter-output combos, and VCS will select the most suitable one (or none) given the current capture and output resolutions.

<dokki-tip>
    <p>
        When deciding which of multiple filter chains to use, VCS will prefer more specific chains to more general ones.
    </p>
    <p>
        If you have e.g. an input gate whose width and height are 0, and another input gate whose width and height are 640 and 480, the latter will be used when the capture resolution is exactly 640 &times; 480, and the former otherwise.
    </p>
    <p>
        Likewise, if your input gates are 0 &times; 0 and 640 &times; 0, the former will be applied for capture resolutions of <i>any</i> &times; <i>any</i>, except for 640 &times; <i>any</i>, where the latter chain will apply &ndash; except if you also have a third input gate of 640 &times; 480, in which case that will be used when the capture resolution is exactly 640 &times; 480.
    </p>
</dokki-tip>

To connect two nodes, click and drag with the left mouse button from one node's output edge (square) to another's input edge (circle), or vice versa. A node can be connected to as many other nodes as you like. To disconnect a node from another, right-click on the node's output edge, and select the other node from the list that pops up. To remove a node itself from the graph, right-click on the node and select to remove it. To add nodes to the graph, right-click on the graph's background to bring up the node menu, or select <menu-path>Filters</menu-path> from the dialog's menu bar.

## Input resolution dialog

This dialog can be accessed with <key-combo>Ctrl + I</key-combo> or <menu-path>Context > Input > Resolution...</menu-path>.

Normally, the capture device will automatically set the capture resolution to match that of the input signal, but sometimes the result isn't quite right. The input resolution dialog lets you override this resolution with your own one.

You can change a button's assigned resolution by clicking on it while pressing the <key-combo>Alt</key-combo> key.

## Output size dialog

This dialog can be accessed with <key-combo>Ctrl + O</key-combo> or <menu-path>Context > Output > Size...</menu-path>.

The dialog lets you resize the [output window](#output-window).

### Settings explained

<dokki-table headerless>
    <table>
        <tr>
            <th>Setting</th>
            <th>Description</th>
        </tr>
        <tr>
            <td>Constant</td>
            <td>
                Lock the size of the output window so that changes to the capture resolution don't affect the output
                resolution. Frames will be scaled up or down as needed to match this resolution.
            </td>
        </tr>
        <tr>
            <td>Scale</td>
            <td>
                Scale the size of the output window up or down by a percentage of its base size. The base size is
                either the capture resolution, or, if enabled, the <strong>Constant</strong> resolution.
            </td>
        </tr>
    </table>
</dokki-table>

## Overlay editor dialog

This dialog can be accessed with <key-combo>Ctrl + L</key-combo> or <menu-path>Context > Output > Overlay editor...</menu-path>.

> The overlay editor dialog (right) showing the HTML/CSS source of an overlay (top left) in this capture of a Linux desktop\
![{image}{headerless}](../img/vcs-2.6-overlay.webp)

The overlay dialog lets you define a message to be overlaid on the [output window](#output-window), with optional HTML/CSS styling.

### Formatting

For a list of the HTML and CSS formatting features supported, see [Qt5: Supported HTML Subset](https://doc.qt.io/qt-5/richtext-html-subset.html).

> A sample overlay with HTML/CSS formatting that displays information about the current video mode
```html [{headerless}]
<style>
    video-mode {
        color: yellow;
        background-color: black;
        font-family: "Comic Sans MS";
    }
</style>

<video-mode>
    Input: $inWidth &times; $inHeight, $inRate Hz
</video-mode>
```

> A sample overlay without HTML/CSS formatting that displays information about the current video mode
```text [{headerless}]
Input: $inWidth x $inHeight, $inRate Hz
```

### Variables

The following variables will expand to dynamic values in the overlay:

<dokki-table headerless>
    <table>
        <tr>
            <th>Variable</th>
            <th>Explanation</th>
        </tr>
        <tr>
            <td><em>$inWidth</em></td>
            <td>The width of the current input resolution.</td>
        </tr>
        <tr>
            <td><em>$inHeight</em></td>
            <td>The height of the current input resolution.</td>
        </tr>
        <tr>
            <td><em>$inRate</em></td>
            <td>The refresh rate of the current input signal.</td>
        </tr>
        <tr>
            <td><em>$inChannel</em></td>
            <td>The index of the current input channel on the capture device.</td>
        </tr>
        <tr>
            <td><em>$outWidth</em></td>
            <td>The width of the current output resolution.</td>
        </tr>
        <tr>
            <td><em>$outHeight</em></td>
            <td>The height of the current output resolution.</td>
        </tr>
        <tr>
            <td><em>$outRate</em></td>
            <td>The current output frame rate.</td>
        </tr>
        <tr>
            <td><em>$frameDropIndicator</em></td>
            <td>Shows a warning label if frames are currently being dropped, or nothing if they aren't.</td>
        </tr>
        <tr>
            <td><em>$time</em></td>
            <td>Current system time.</td>
        </tr>
        <tr>
            <td><em>$date</em></td>
            <td>Current system date.</td>
        </tr>
    </table>
</dokki-table>

## Video presets dialog

This dialog can be accessed with <key-combo>Ctrl + V</key-combo> or <menu-path>Context > Input > Video presets...</menu-path>.

> A screenshot of the video presets dialog\
![{image:513x656}{headerless}](../img/video-presets-dialog.webp)

The video presets dialog lets you to modify the capture devices's video signal parameters.

A given video preset's parameters will be applied when all of its "Activates with" conditions are met. For instance, if you've defined a preset's activation resolution as 800 &times; 600 and have disabled the other activating conditions, the preset's parameters will be applied when the capture video mode is 800 &times; 600.

To add or delete a preset, click the + or - buttons next to the preset selector at the top of the dialog. Clicking the + button while holding the Alt key will create a new preset with the current preset's settings.

If you want your changes to the video presets to persist after you exit VCS, remember to save them first! This can be done via <menu-path>File > Save as&hellip;</menu-path>. Saved settings can be restored via <menu-path>File > Open&hellip;</menu-path>. Any saved settings that're open when VCS exits will be reloaded automatically when you run VCS again.
