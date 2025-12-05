#!/usr/bin/env python3
import sys
import os
import json

def extract_sound_name(entry):
    if not entry:
        return "Unknown"

    props = entry.get("Properties", {})

    for key in ("Event_FP", "Event_TP"):
        if key in props and isinstance(props[key], dict):
            n = props[key].get("ObjectName")
            if n and n != "None":
                return n

    return entry.get("Name", "Unknown")


def process_json(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    anim = next((i for i in data if i.get("Type") in ("AnimSequence","AnimMontage")), None)
    if not anim:
        return False, "AnimSequence/AnimMontage not found"

    props = anim.get("Properties", {})
    notifies = props.get("Notifies", [])
    num_frames = props.get("NumFrames", 0)
    seq_length = props.get("SequenceLength", 1)

    fps = (num_frames / seq_length) if seq_length != 0 else 0

    sound_map = {
        f"AnimNotify_WeaponSound'{e.get('Outer')}:{e.get('Name')}'": e
        for e in data if e.get("Type") == "AnimNotify_WeaponSound"
    }

    result = []
    for n in notifies:
        event_type = n.get("NotifyName", "Unknown")
        t = n.get("Time", n.get("LinkValue", 0))
        frame = int(t * fps)

        if event_type == "WeaponSound":
            ref = n.get("Notify", {}).get("ObjectName", "")
            name = extract_sound_name(sound_map.get(ref))
        else:
            name = event_type

        result.append(f"{t:.6f}\t{frame}\t{event_type}\t{name}")

    if not result:
        return False, "No notifies found"

    out = os.path.splitext(path)[0] + "_events.txt"
    with open(out, "w", encoding="utf-8") as f:
        f.write("Time(sec)\tFrame\tEvent\tDetails\n")
        f.write("\n".join(result))

    return True, f"Saved → {out}"


def main():
    if len(sys.argv) < 2:
        print("\nDrag & Drop JSON onto this EXE\n")
        return

    path = sys.argv[1]
    if not os.path.exists(path) or not path.lower().endswith(".json"):
        print("Invalid or non-json file.")
        return

    ok, msg = process_json(path)
    print(msg)


if __name__ == "__main__":
    main()
