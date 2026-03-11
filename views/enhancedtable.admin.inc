<?php

/**
 * Flipped renderer for the table style plugin UI.
 *
 * - Columns = fields (Title, Description, ...)
 * - Rows    = options (Column, Align, Separator, Click-sorting, Hide if empty)
 * - "Click-sorting" row shows:
 *     * Setting column: "Click-sorting:" then "No default  ○"
 *     * Field columns:  "□ sortable  [order]  ○ default"
 * - Rest of the style-options form is rendered below the matrix.
 */
function theme_enhancedtable_flipped_options_table($vars) {
  $form = $vars['form'];

  $build = array();

  // Preview (currently disabled by options_form(), but kept for later).
  if (isset($form['enhancedtable_preview'])) {
    $build['preview'] = $form['enhancedtable_preview'];
  }

  if (!empty($form['columns']) && is_array($form['columns'])) {
    // Use the visible field list from options_form() when available.
    if (!empty($form['#enhancedtable_fields']) && is_array($form['#enhancedtable_fields'])) {
      $fields = $form['#enhancedtable_fields'];
    }
    else {
      // Fallback: all non-meta keys from columns.
      $fields = array();
      foreach ($form['columns'] as $key => $child) {
        if ($key === '' || $key[0] === '#') {
          continue;
        }
        $fields[] = $key;
      }
    }

    // Normalize titles for sortable/default radios.
    foreach ($fields as $field) {
      if (isset($form['info'][$field]['sortable'])) {
        $form['info'][$field]['sortable']['#title'] = t('sortable');
        $form['info'][$field]['sortable']['#title_display'] = 'after';
      }
      if (isset($form['default'][$field])) {
        $form['default'][$field]['#title'] = t('default');
        $form['default'][$field]['#title_display'] = 'after';
      }
    }
    // For the "No default" radio we will supply the text ourselves.
    if (isset($form['default'][-1])) {
      $form['default'][-1]['#title'] = '';
      $form['default'][-1]['#title_display'] = 'after';
    }

    // Header: "Setting" + one column per field.
    $header = array();
    $header[] = t('Setting');
    foreach ($fields as $field) {
      $label = isset($form['info'][$field]['name']['#markup'])
        ? $form['info'][$field]['name']['#markup']
        : check_plain($field);
      $header[] = $label;
    }

    $rows = array();

    // Helper for normal rows (column, align, separator, hide-if-empty).
    $add_row = function($label, $callback) use (&$rows, $fields) {
      $cells = array();
      $cells[] = array('data' => array('#markup' => $label));
      $has_content = FALSE;

      foreach ($fields as $field) {
        $element = $callback($field);
        if ($element !== NULL) {
          $cells[] = array('data' => backdrop_render($element));
          $has_content = TRUE;
        }
        else {
          $cells[] = array('data' => '');
        }
      }

      if ($has_content) {
        $rows[] = array('data' => $cells);
      }
    };

    // 1. Column row.
    $add_row(t('Column'), function($field) use ($form) {
      return isset($form['columns'][$field]) ? $form['columns'][$field] : NULL;
    });

    // 2. Separator row.
    $add_row(t('Separator'), function($field) use ($form) {
      return isset($form['info'][$field]['separator']) ? $form['info'][$field]['separator'] : NULL;
    });

    // 3. Alignment row.
    $add_row(t('Alignment'), function($field) use ($form) {
      return isset($form['info'][$field]['align']) ? $form['info'][$field]['align'] : NULL;
    });

    // 4. Width row (uses column_widths[FIELD] if present) – under Alignment.
    if (!empty($form['column_widths']) && is_array($form['column_widths'])) {
      $add_row(t('Width'), function($field) use (&$form) {
        if (!isset($form['column_widths'][$field])) {
          return NULL;
        }
        // Hide per-field label; header already shows the field name.
        $form['column_widths'][$field]['#title_display'] = 'invisible';
        return $form['column_widths'][$field];
      });
    }

    // 5. Click-sorting row.
    $has_sortable = FALSE;
    $click_cells = array();

    // First cell: "Click-sorting:" + "No default  ○".
    $no_default_html = '';
    if (isset($form['default'][-1])) {
      // Render the "no default" radio once.
      $no_default_html = backdrop_render($form['default'][-1]);
    }

    $label_markup = '<div class="bt-click-sorting-label">'
      . '<div>' . check_plain(t('Click-sorting:')) . '</div>';

    if ($no_default_html !== '') {
      $label_markup .= '<div class="bt-no-default">'
        . check_plain(t('No default'))
        . ' '
        . $no_default_html
        . '</div>';
    }

    $label_markup .= '</div>';

    $click_cells[] = array('data' => array('#markup' => $label_markup));

    // Field cells: "□ sortable  ○ default  [order]" in that order.
    foreach ($fields as $field) {
      if (!isset($form['info'][$field]['sortable'])) {
        $click_cells[] = array('data' => '');
        continue;
      }

      $has_sortable = TRUE;

      // Render each control separately so we control their order.
      $sortable_html = backdrop_render($form['info'][$field]['sortable']);
      $order_html = '';
      if (isset($form['info'][$field]['default_sort_order'])) {
        $order_html = backdrop_render($form['info'][$field]['default_sort_order']);
      }
      $default_html = '';
      if (isset($form['default'][$field])) {
        $default_html = backdrop_render($form['default'][$field]);
      }

      $cell_markup = '<div class="bt-sortable-wrapper">'
        . $sortable_html
        . ($default_html ? ' ' . $default_html : '')
        . ($order_html ? ' ' . $order_html : '')
        . '</div>';

      $click_cells[] = array('data' => array('#markup' => $cell_markup));
    }

    if ($has_sortable) {
      $rows[] = array('data' => $click_cells);
    }

    // 6. Hide-if-empty row.
    $add_row(t('Hide if empty'), function($field) use ($form) {
      return isset($form['info'][$field]['empty_column']) ? $form['info'][$field]['empty_column'] : NULL;
    });

    $build['flipped_table'] = array(
      '#theme'      => 'table',
      '#header'     => $header,
      '#rows'       => $rows,
      '#attributes' => array('class' => array('bt-matrix', 'sticky-enabled')),
    );
  }

  // Rest of the style-options form (grouping, row classes, etc.).
  $rest = $form;
  unset(
    $rest['columns'],
    $rest['info'],
    $rest['default'],
    $rest['enhancedtable_preview'],
    $rest['column_widths'],   // Do NOT render the standalone fieldset anymore.
    $rest['#enhancedtable_fields']
  );

  $build['rest'] = array(
    '#markup' => backdrop_render_children($rest),
  );

  return backdrop_render($build);
}

/**
 * Theme function for EnhancedTable's cards-only UI in the Views style options.
 *
 * This intentionally does NOT render the stock Views UI table matrix.
 * The EnhancedTable style plugin builds its own UI elements (bt_layout, etc.)
 * and translates them back into the standard table style options on submit.
 */
function theme_enhancedtable_views_ui_style_plugin_table($vars) {
  $form = $vars['form'];

  $output = '';
  if (isset($form['description_markup'])) {
    $output .= backdrop_render($form['description_markup']);
  }

  // Do not render legacy table-matrix elements if they exist.
  if (isset($form['columns']) && is_array($form['columns'])) {
    $form['columns']['#access'] = FALSE;
  }
  if (isset($form['info']) && is_array($form['info'])) {
    $form['info']['#access'] = FALSE;
  }
  if (isset($form['default']) && is_array($form['default'])) {
    $form['default']['#access'] = FALSE;
  }

  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * Helper: build one collapsible help section.
 */
function enhancedtable_build_help_section($title, $markup, $collapsed = TRUE, $weight = 0) {
  return array(
    '#type' => 'fieldset',
    '#title' => $title,
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
    '#weight' => $weight,
    'text' => array(
      '#type' => 'markup',
      '#markup' => $markup,
    ),
  );
}

/**
 * Build shared help UI for EnhancedTable-style settings screens.
 *
 * @return array
 *   A Backdrop Form API render array.
 */
function enhancedtable_build_shared_help_form() {
  $help = array(
    '#type' => 'fieldset',
    '#title' => t('Table feature help'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 0,
    '#attributes' => array('class' => array('enhancedtable-help')),
    '#description' => t('Guidance for column layout, combining fields, sorting, widths, borders, and row classes.'),
  );

  $help['column_ordering'] = enhancedtable_build_help_section(t('Column ordering'),
    '<p>' . t('Use the left/right controls to change the order of columns in the rendered table.') . '</p>'
    . '<p>' . t('This affects display only. It does not change the query order, field order, or data retrieval order defined in the Fields → Rearrange section of the Views UI.') . '</p>', TRUE, 0
  );

  $help['combining_columns'] = enhancedtable_build_help_section(t('Combining columns'),
    '<p>' . t('To combine one field into the column to its left, place the field immediately to the right of the destination column and click <em>Merge left</em>.') . '</p>'
    . '<p>' . t('When a field is merged left:') . '</p>'
    . '<ul>'
    . '<li>' . t('its content is displayed in the cell of the column to its left') . '</li>'
    . '<li>' . t('only the header of the left-hand column is shown') . '</li>'
    . '<li>' . t('the merged field no longer renders as a separate column') . '</li>'
    . '</ul>'
    . '<p>' . t('You may enter separator text to appear between merged values. HTML is allowed in the separator field. For example, use a space, a dash, or <code>&lt;br&gt;</code> to place the second value on a new line.') . '</p>'
    . '<p>' . t('Select <em>Inline title</em> to prepend the field label to its rendered value inside the table cell.') . '</p>'
    . '<p><strong>' . t('Notes:') . '</strong></p>'
    . '<ul>'
    . '<li>' . t('A field can only be merged into the column immediately to its left.') . '</li>'
    . '<li>' . t('If several adjacent fields are merged left, they will all render in the same destination column, in display order.') . '</li>'
    . '</ul>', TRUE, 10
  );

  $help['alignment'] = enhancedtable_build_help_section(t('Alignment'),
    '<p>' . t('Sets the horizontal alignment of both the column header and the cell contents for that column.') . '</p>'
    . '<p>' . t('This affects rendered table output only.') . '</p>', TRUE, 20
  );

  $help['width'] = enhancedtable_build_help_section(t('Width'),
    '<p>' . t('Specify a preferred width for each column. The browser will honor the width when possible.') . '</p>'
    . '<p>' . t('Supported units include: px, em, rem, %, ch, ex.') . '</p>'
    . '<p><strong>' . t('Notes:') . '</strong></p>'
    . '<ul>'
    . '<li>' . t('Column width in HTML tables is advisory, not absolute.') . '</li>'
    . '<li>' . t('On narrow screens or crowded tables, the browser may reduce a column below the specified width in order to fit the table.') . '</li>'
    . '<li>' . t('Width settings apply to the rendered column. If several fields are merged into one column, the width applies to the destination column.') . '</li>'
    . '</ul>', TRUE, 30
  );

  $help['click_sorting'] = enhancedtable_build_help_section(t('Click sorting'),
    '<p>' . t('For sortable fields, you may enable click sorting and choose the sort direction used on the first click.') . '</p>'
    . '<p>' . t('You may also choose a default click-sort column for the initial table display, or select <em>No default</em> to leave the table without a default click-sort column on first load.') . '</p>'
    . '<p><strong>' . t('Notes:') . '</strong></p>'
    . '<ul>'
    . '<li>' . t('Click sorting affects the order of rows in the rendered view.') . '</li>'
    . '<li>' . t('If normal Views sort criteria are also present, the selected click-sort behavior determines whether those criteria are ignored or appended as tie-breakers.') . '</li>'
    . '<li>' . t('Click sorting is available only for fields that support sorting in Views.') . '</li>'
    . '</ul>', TRUE, 40
  );

  $help['hide_if_empty'] = enhancedtable_build_help_section(t('Hide if empty'),
    '<p>' . t('Hide this column when the view returns no data for this field.') . '</p>'
    . '<p>' . t('This is most useful when a field is optional and would otherwise produce an empty column across the result set.') . '</p>', TRUE, 50
  );

  $help['borders'] = enhancedtable_build_help_section(t('Borders'),
    '<p>' . t('Specify border thickness for table frame, rows, and columns.') . '</p>'
    . '<p>' . t('Supported units include: px, em, rem, ch, ex.') . '</p>'
    . '<p><strong>' . t('Notes:') . '</strong></p>'
    . '<ul>'
    . '<li>' . t('Border settings apply to the rendered table only.') . '</li>'
    . '<li>' . t('The visible result may vary somewhat depending on browser table layout behavior and theme CSS.') . '</li>'
    . '</ul>', TRUE, 60
  );

  $help['add_views_row_classes'] = enhancedtable_build_help_section(t('Add Views row classes'),
    '<p>' . t('Adds the default Views row classes, such as <code>views-row</code>, <code>views-row-1</code>, <code>views-row-first</code>, <code>views-row-last</code>, <code>odd</code>, and <code>even</code>.') . '</p>'
    . '<p>' . t('Disable this to reduce markup output.') . '</p>'
    . '<p><strong>' . t('Note:') . '</strong> ' . t('Your theme or custom CSS may rely on these classes. If you disable them, check the rendered result carefully.') . '</p>', TRUE, 70
  );

  $help['row_class'] = enhancedtable_build_help_section(t('Row class'),
    '<p>' . t('A CSS class to add to each table row.') . '</p>'
    . '<p>' . t('Use this to target the rows of this view with custom CSS.') . '</p>', TRUE, 80
  );

  return $help;
}

