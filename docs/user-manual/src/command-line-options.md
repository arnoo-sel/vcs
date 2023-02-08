# Command-line options

<dokki-table headerless>
    <table>
        <tr>
            <th>Option</th>
            <th>Description</th>
        </tr>
        <tr>
            <td>-i <i>&lt;input channel&gt;</i></td>
            <td>
                Start capture on the given input channel (1&#8230;<i>n</i>). On Linux, a value of 1 corresponds to <em>/dev/video0</em>, 2 to <em>/dev/video1</em>, 3 to <em>/dev/video2</em>, and so on. By default, channel 1 will be used.
            </td>
        </tr>
        <tr>
            <td>-v <i>&lt;path&gt;</i></td>
            <td>
                Load video presets from the given file on start-up. Video preset files typically have the .vcs-video suffix.
            </td>
        </tr>
        <tr>
            <td>-f <i>&lt;path&gt;</i></td>
            <td>
                Load a custom filter graph from the given file on start-up. Filter graph files typically have the .vcs-filter-graph suffix.
            </td>
        </tr>
        <tr>
            <td>-a <i>&lt;path&gt;</i></td>
            <td>
                Load alias resolutions from the given file on start-up. Alias resolution files typically have the .vcs-alias suffix.
            </td>
        </tr>
        <tr>
            <td>-m <i>&lt;value in MB&gt;</i></td>
            <td>
                Set the amount of system memory that VCS reserves on startup. If you're getting error messages about the memory cache running out, increase this value. If you get x264 allocation errors when attempting to record video, try reducing this value. Default: 256 MB.
            </td>
        </tr>
        <tr>
            <td>-s</td>
            <td>
                Don't prevent the screensaver from activating while VCS is running.
            </td>
        </tr>
    </table>
</dokki-table>
