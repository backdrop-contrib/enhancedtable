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
