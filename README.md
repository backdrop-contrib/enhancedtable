# Views Enhanced Table

Views Enhanced Table provides an enhanced **Table** style plugin for Backdrop Views.

It keeps the core table output and options, but adds a more convenient configuration UI and several additional display controls.

## Features

### Columns
- **Presentation-only column ordering** (left/right controls).
- **Combine columns** (“Merge left”) so multiple fields render under one table header.
  - Supports **separator text** between merged fields (HTML allowed; e.g. `<br>` to stack values).
- **Per-column alignment** (applies to header + cells).
- **Per-column width** (CSS width value; validated).
- **Hide if empty** per column.

### Click-sorting
- Enable click-sorting for sortable fields.
- Choose **default** click-sort field and initial order.

### Styling
- Optional **row striping** (core table option).
- Optional borders:
  - **Horizontal row borders**
  - **Vertical column borders**
  - **Outer table frame**
  - Thickness and color controls.
- Optional “Add views row classes” (e.g. `views-row-1`).

### Organization
- Style options are grouped into fieldsets:
  - **Columns**, **Styling**, **Mechanics**, **Admin**, plus **Help**.

## Requirements

- Backdrop CMS 1.x
- Views (core module)

## Installation

1. Place the module directory in `modules/enhancedtable` (or `modules/custom/enhancedtable`).
2. Enable **Views Enhanced Table** at `admin/modules`.

## Usage

1. Edit a View.
2. In the display’s **Format** section, choose:
   - **Show:** Table
   - **Style plugin:** **Enhanced table**
3. Click **Settings** next to the style plugin.
4. Use the **Columns** fieldset to reorder, merge, set separators, alignment, widths, and click-sorting.
5. Use **Styling** for borders/striping and row-class options.

## Configuration storage

Views Enhanced Table does not create its own module-level configuration object.

All settings are stored **per display** inside the View’s configuration (the display’s `style_plugin` and `style_options`).

## Disabling / uninstalling

If a View display is configured with the **Enhanced table** style plugin and the module is disabled, Views cannot load the missing plugin and that display may render no output.

Before disabling/uninstalling, switch affected displays back to the core **Table** style plugin.

(Planned improvement: an admin utility to audit/convert displays that use EnhancedTable.)

## Notes

- Column widths and border thickness are validated to conservative CSS length patterns (e.g. `120px`, `10em`, `25%`).
- Separators accept HTML. This option is intended for trusted administrators.

## Issues

- Please report bugs and feature requests in the project's issue queue.
- Include Backdrop core version, PHP version, and steps to reproduce.

## Maintainers

- Eric Foy (project maintainer)

## License

GPL-2.0-or-later (same terms as Backdrop CMS).
