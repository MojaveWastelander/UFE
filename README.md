# UFE
Underrail File Exporter  
Tool that attempts to export binary serialized data of Underrail files to more user friendly json format.  
Resulting json file can then be used to patch original binary file (update certain fields).  
It is practically a more user-friendly alternative to using a hex editor to update certain values and find more information about items/quests as the 
.NET stream file format used is hard to reorganize to have a more practical modding tool.

# Example usage
- Export single file
```
❯ UFE -e x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:47:11][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:47:11][info] File parsed successfully 
[12:47:11][info] Exporting data to 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item.json'
```
- Export all files in a directory and all subdirectories
```
❯ UFE -e x:\Games\GOG\UnderRail\data\rules\items\armor
[12:50:09][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item
[12:50:09][info] File parsed successfully
[12:50:09][info] Exporting data to 'x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item.json'
[12:50:09][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:50:09][info] File parsed successfully
[12:50:09][info] Exporting data to 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item.json'
[12:50:09][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardsuit.item
[12:50:09][info] File parsed successfully
[12:50:09][info] Exporting data to 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardsuit.item.json'
...
```
- Patch single file
```
❯ UFE -p x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:53:29][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:53:29][info] File parsed successfully
[12:53:29][info] Patching file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item'
[12:53:29][info] with json file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item.json'
[12:53:29][warning] orig_str: These boots protect the feet from most conventional and unconventional biohazards.
[12:53:29][warning] json_str: These heavy boots protect the feet from most conventional and unconventional biohazards.
[12:53:29][warning]     '2' ==> '8'
[12:53:29][warning]     '0.25' ==> '0.5'
[12:53:29][warning]     '0.25' ==> '0.5'
```
- Patch all files in a directory and all subdirectories
```
❯ UFE -p x:\Games\GOG\UnderRail\data\rules\items\armor
[12:54:48][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item
[12:54:48][info] File parsed successfully
[12:54:48][info] Patching file 'x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item'
[12:54:48][info] with json file 'x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item.json'
[12:54:48][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:54:48][info] File parsed successfully
[12:54:48][info] Patching file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item'
[12:54:48][info] with json file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item.json'
[12:54:48][warning] orig_str: These boots protect the feet from most conventional and unconventional biohazards.
[12:54:48][warning] json_str: These heavy boots protect the feet from most conventional and unconventional biohazards.
[12:54:48][warning]     '2' ==> '8'
[12:54:48][warning]     '0.25' ==> '0.5'
[12:54:48][warning]     '0.25' ==> '0.5'
[12:54:48][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardsuit.item
[12:54:48][info] File parsed successfully
[12:54:48][info] Patching file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardsuit.item'
[12:54:48][info] with json file 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardsuit.item.json'
[12:54:48][warning]     '3' ==> '4'
[12:54:48][warning]     '1500' ==> '3500'
...
```
- Validate all files in a directory and all subdirectories
```
❯ UFE -v x:\Games\GOG\UnderRail\data\rules\items\armor
[12:55:46][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item
[12:55:46][info] File parsed successfully
[12:55:46][info] File 'x:\Games\GOG\UnderRail\data\rules\items\armor\advancedcatalyzingbelt.item' is compressed, validation status 'validated'
[12:55:46][info] Reading file: x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item
[12:55:46][info] File parsed successfully
[12:55:46][info] File 'x:\Games\GOG\UnderRail\data\rules\items\armor\biohazardboots.item' is compressed, validation status 'validated'
...
```

# Building
Install and setup [vcpkg](https://github.com/microsoft/vcpkg), then install following dependencies
```
❯ vcpkg install nlohmann-json cli spdlog gzip-hpp --triplet=x64-windows
```
Open solution in latest Visual Studio version and build the executable
