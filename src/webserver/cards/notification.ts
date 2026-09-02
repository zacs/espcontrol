import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import {
    defaultNotificationLevelAttribute,
    normalizeNotificationOptions,
    notificationAckAction,
    notificationAckActionValid,
    notificationLevelAttribute,
    notificationMessageAttribute,
    setNotificationAckAction,
    setNotificationLevelAttribute,
    setNotificationMessageAttribute,
} from "../application/config_notification_contract";
import { NOTIFICATION_LEVEL_COLORS } from "../state/ui_tokens";
import type { CardRegistry, CardUiServices } from "../application/card_registry";

// Level, human label, and the icon the device shows on the card.
const NOTIFICATION_LEVEL_LEGEND: readonly (readonly [keyof typeof NOTIFICATION_LEVEL_COLORS, string, string])[] = [
    ["information", "Information", "information"],
    ["warning", "Warning", "alert"],
    ["alert", "Alert", "alert-octagon"],
];

export function registerNotificationCardTypes(
    registry: CardRegistry,
    document: Document,
    cardUi: CardUiServices,
): void {
    const { renderPreview } = cardUi;
    // Read-only card that shows a Home Assistant message and colours itself
    // from a severity attribute on the same entity.
    const NOTIFICATION_CARD_METADATA: any = {
        entity: {
            label: "Message Entity",
            idSuffix: "entity",
            placeholder: "e.g. sensor.house_notice",
            domains: function (this: any) { return cardContractDomains("notification"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add a sensor entity before saving.",
        },
    };
    function renderLevelLegend(this: any, panel?: any) {
        var legend: any = document.createElement("div");
        legend.className = "sp-field sp-notification-legend";
        NOTIFICATION_LEVEL_LEGEND.forEach(function (this: any, entry?: any) {
            var colors: any = NOTIFICATION_LEVEL_COLORS[entry[0] as keyof typeof NOTIFICATION_LEVEL_COLORS];
            var chip: any = document.createElement("span");
            chip.className = "sp-notification-chip";
            chip.style.backgroundColor = "#" + colors.background;
            chip.style.color = "#" + colors.text;
            var glyph: any = document.createElement("span");
            glyph.className = "mdi mdi-" + entry[2];
            chip.appendChild(glyph);
            chip.appendChild(document.createTextNode(entry[1]));
            legend.appendChild(chip);
        });
        panel.appendChild(legend);
    }
    function bindOptionField(this: any, control?: any, b?: any, helpers?: any, apply?: any) {
        function save(this: any) {
            apply(b, control.input.value);
            control.input.value = control.input.value.trim();
            helpers.saveField("options", b.options);
            renderPreview();
        }
        control.input.addEventListener("change", save);
        control.input.addEventListener("blur", save);
    }
    registry.register("notification", {
        label: function (this: any) { return cardContractCardLabel("notification"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("notification"); },
        pickerKey: function (this: any) { return cardContractPickerKey("notification"); },
        hidden: function (this: any) { return cardContractHidden("notification"); },
        labelPlaceholder: "e.g. House Notice",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("notification"); },
        cardMetadata: NOTIFICATION_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = normalizeNotificationOptions(b.options);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            void slot;
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = normalizeNotificationOptions(b.options);
            helpers.renderCardEntityField(panel, b, helpers, NOTIFICATION_CARD_METADATA);
            renderLevelLegend(panel);
            var disclosure: any = helpers.disclosureSection(
                "Notification Settings", helpers.idPrefix + "notification-settings", false);
            var levelField: any = helpers.textField(
                "Level Attribute",
                helpers.idPrefix + "notification-level-attribute",
                notificationLevelAttribute(b),
                defaultNotificationLevelAttribute(),
                null,
                false,
            );
            disclosure.section.appendChild(levelField.field);
            bindOptionField(levelField, b, helpers, setNotificationLevelAttribute);
            var messageField: any = helpers.textField(
                "Message Attribute",
                helpers.idPrefix + "notification-message-attribute",
                notificationMessageAttribute(b),
                "Uses the entity state when blank",
                null,
                false,
            );
            disclosure.section.appendChild(messageField.field);
            bindOptionField(messageField, b, helpers, setNotificationMessageAttribute);
            var ackField: any = helpers.textField(
                "Acknowledge Action",
                helpers.idPrefix + "notification-ack-action",
                notificationAckAction(b),
                "e.g. script.acknowledge_notice",
                null,
                false,
            );
            disclosure.section.appendChild(ackField.field);
            bindOptionField(ackField, b, helpers, function (this: any, button?: any, value?: any) {
                setNotificationAckAction(button, value);
                if (value && !notificationAckActionValid(value)) {
                    helpers.showFieldError(ackField.input, "Use a Home Assistant action such as script.acknowledge_notice.");
                }
                else {
                    helpers.clearFieldError(ackField.input);
                }
            });
            panel.appendChild(disclosure.panel);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            // The live message and its level only exist on the device, so the
            // preview stands in the configured label and the information
            // colours, laid out the way the card renders: the severity icon
            // above one centred line of text.
            var label: any = String((b && b.label) || (b && b.entity) || "Notification").trim();
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-information"></span>',
                labelHtml: '<span class="sp-notification-message">' + helpers.escHtml(label) + "</span>",
            };
        },
    });
}
