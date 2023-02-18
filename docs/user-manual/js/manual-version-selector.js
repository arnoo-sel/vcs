/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: Version selector widget for VCS User's Manual
 * 
 * 
 * Intended for use as a custom widget in VCS's dokki-based user's manual.
 * 
 * The widget presents the user a selection box for choosing which version of the
 * manual to view.
 *
 */

const manualVersionSelector = {
    $tag: "manual-version-selector",
    data() {
        return {
            inlineSvgFillColor: "currentColor",
        };
    },
    computed: {
        supported_manual_versions() {
            return [
                "master",
                "2.7.0",
                "2.6.0",
                "2.5.2",
                "2.5.1",
                "2.5.0",
                "2.4.0",
            ];
        },
        current_manual_version() {
            return window.VCS_MANUAL_VERSION;
        },
    },
    methods: {
        update_inline_svg_fill_color() {
            this.inlineSvgFillColor = (
                getComputedStyle(document.body)
                .getPropertyValue("--dokkiCSS-page-secondary-fg-color")
                .replace("#", "%23")
            );
        },
        on_select(event) {
            const newUrl = (
                (event.target.value === "master")
                ? "/vcs/docs/user-manual/"
                : `/vcs/docs/user-manual/${event.target.value}/`
            );

            window.history.replaceState(null, "", newUrl);
            window.location.reload();
        },
        is_supported_manual_version(version = "") {
            return this.supported_manual_versions.includes(version);
        },
    },
    created() {
        this.update_inline_svg_fill_color();

        new MutationObserver((mutations)=>{
            if (mutations.some(m=>m.attributeName === "data-dokki-theme")) {
                this.update_inline_svg_fill_color();
            }
        }).observe(document.body, {
            attributes: true, childList: false, characterData: false
        });
    },
    template: `
    <dokki-user-widget
        v-if="is_supported_manual_version(current_manual_version)"
        class="manual-version-selector"
        title="Select a different version of VCS"
    >
        <select @change="on_select">
            <option
                v-for="version in supported_manual_versions"
                :selected="version === current_manual_version"
            >
                {{version}}
            </option>
        </select>
    </dokki-user-widget>

    <component is="style" type="text/css">
        .dokki-user-widget.manual-version-selector
        {
            color: var(--dokkiCSS-page-secondary-fg-color);
            font-family: var(--dokkiCSS-regular-font-family);
            font-size: var(--dokkiCSS-regular-font-size);
            border-radius: var(--dokkiCSS-embedded-border-radius);
            padding: min(10px, 0.4em);
            margin-top: max(-11px, calc(-1 * (0.4em + 1px))) !important;
            margin-bottom: max(-11px, calc(-1 * (0.4em + 1px))) !important;
            border: 1px solid var(--dokkiCSS-page-primary-line-color);
        }

        .dokki-user-widget.manual-version-selector i
        {
            margin-right: 0.25em !important;
        }

        .dokki-user-widget.manual-version-selector select
        {
            border: none;
            background-color: var(--dokkiCSS-page-primary-bg-color);
            outline: none;
            border-color: var(--dokkiCSS-page-primary-line-color);
            font-family: inherit;
            font-size: inherit;
            color: inherit;
            appearance: none;
            padding: 0 0.25em;
            padding-right: 1.25em;
            background-image: url("
                data:image/svg+xml;utf8,
                &lt;svg fill='{{inlineSvgFillColor}}' height='24' viewBox='0 0 24 24' width='24' xmlns='http://www.w3.org/2000/svg'&gt;
                &lt;path d='M7 10l5 5 5-5z'/&gt;&lt;path d='M0 0h24v24H0z' fill='none'/&gt;
                &lt;/svg&gt;
            ");
            background-repeat: no-repeat;
            background-position: top 0 right 0;
            background-size: 1.25em auto;
        }

        .dokki-user-widget.manual-version-selector select:hover
        {
            cursor: pointer;
        }
    </component>
    `,
};

window.dokkiUserComponents = (window.dokkiUserComponents || []);
window.dokkiUserComponents.push(manualVersionSelector);
