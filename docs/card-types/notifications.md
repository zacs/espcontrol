---
title: Notification Cards
description:
  How to show a Home Assistant message on your EspControl panel and colour it by
  how urgent it is.
---

# Notification

A Notification card shows one message from a Home Assistant entity and colours itself by how urgent that message is. It suits reminders, service alerts, "the garage has been open for an hour" style warnings, and anything else where the important part is a sentence rather than a number.

Notification cards are display cards. The message is shown on one line, vertically centred, at the same larger text size the Media now-playing card uses, under a card icon that changes with the severity. Tapping the card opens the full message, so a long sentence is always readable even on a small card.

## Setting Up a Notification Card

1. Select a card and change its type to **Notification**.
2. Enter a **Message Entity**, for example `sensor.house_notice`.
3. Optionally set a **Label**. The label is shown until Home Assistant sends the first message.

That is enough for most setups. The remaining settings live under **Notification Settings**:

| Setting | What it does |
|---|---|
| **Level Attribute** | The entity attribute that carries the urgency. Defaults to `notification_level`. |
| **Message Attribute** | Read the message from this attribute instead of the entity's state. Leave blank to use the state. |
| **Acknowledge Action** | A Home Assistant action such as `script.acknowledge_notice`, shown as an **Acknowledge** button in the expanded message. Leave blank for no button. |

## Levels, Colours, and Icons

The level attribute only changes the colours and the card icon; it never changes the wording.

| Level | Card appearance | Icon |
|---|---|---|
| `information` | Normal card colours. This is also used when the attribute is missing or unrecognised. | Information |
| `warning` | Muted yellow card with dark text. | Warning triangle |
| `alert` | Red card with white text. | Alert octagon |

The icon follows the level, so a Notification card has no icon setting of its own.

Common alternatives are recognised too, so you do not have to match the exact word: `warn`, `caution`, and `medium` count as a warning, and `critical`, `error`, `danger`, `severe`, and `high` count as an alert.

The expanded message also names the level in words, so the urgency is still clear if the colours are hard to tell apart.

## Long Messages

Home Assistant limits an entity's state to 255 characters. That is enough for a sentence or two. If your message is longer, put it in an attribute and name that attribute in **Message Attribute**.

The message always stays on one line. When it is wider than the card, it scrolls across as a marquee; when it fits, it simply sits still. Wider card sizes therefore show more of the message at once, and **Ultra Wide** on a five-column screen shows the most. See [Card Sizes](/features/setup#card-sizes).

Tapping the card always shows the whole message, wrapped over as many lines as it needs.

## Example Home Assistant Sensor

```yaml
template:
  - sensor:
      - name: House Notice
        state: "Garage door has been open for 45 minutes"
        attributes:
          notification_level: warning
```

To send a longer message, keep the state short and add the full text as an attribute:

```yaml
template:
  - sensor:
      - name: House Notice
        state: "Garage door open"
        attributes:
          notification_level: warning
          detail: >-
            The garage door has been open for 45 minutes. The side gate is also
            unlocked, and rain is forecast within the hour.
```

Then set **Message Attribute** to `detail`.

## Troubleshooting

| Problem | What to check |
|---|---|
| The card shows the label instead of a message | Check that the entity exists in Home Assistant and has a state. |
| The card stays in the normal colours and information icon | Check that the attribute named in **Level Attribute** exists on the entity and its value is one of the recognised level names. |
| The message is cut off | The message is longer than Home Assistant's 255-character state limit. Move it to an attribute and set **Message Attribute**. |
| The message scrolls when you would rather it sat still | Use a wider card size, or shorten the message, so it fits the card on one line. |
| The **Acknowledge** button is missing | The action must be written as `domain.service`, for example `script.acknowledge_notice`. Anything else is ignored. |
