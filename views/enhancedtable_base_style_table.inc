<?php
/**
 * File: enhancedtable/views/enhancedtable_base_style_table.inc
 *
 * Shared base class for EnhancedTable-style table features.
 */

class enhancedtable_base_style_table extends views_plugin_style_table {

  /**
   * Whether inline-title prefixes have already been applied to rendered fields.
   *
   * Views may call render_fields() multiple times during a single build.
   * Since Views caches rendered output in $this->rendered_fields, we must
   * ensure we only prefix once to avoid duplicate titles.
   *
   * @var bool
   */
  protected $enhancedtable_inline_titles_applied = FALSE;

  /**
   * Inherit the standard table options.
   */
  public function option_definition() {
    $options = parent::option_definition();

    // Presentation-only field order (left-to-right). Values are numeric weights
    // keyed by field ID.
    $options['field_order'] = array(
      'default' => array(),
    );

    // Presentation-only "merge into the column to the left" flags.
    // Keyed by field ID.
    $options['merge_left'] = array(
      'default' => array(),
    );

    // Presentation-only: when a field is merged into the column to its left,
    // optionally prefix that field's output with its label ("Inline title").
    // Keyed by field ID.
    $options['inline_title'] = array(
      'default' => array(),
    );

    // New: per-column width settings.
    $options['column_widths'] = array(
      'default' => array(),  // keyed by field ID, e.g. ['title' => '10em'].
    );

    // New: global border toggles.
    $options['row_borders'] = array(
      'default' => 0,
    );
    $options['column_borders'] = array(
      'default' => 0,
    );
    $options['frame_border'] = array(
      'default' => 0,
    );
    // NEW: border thickness & color (with conservative defaults).
    $options['row_border_thickness'] = array(
      'default' => '1px',
    );
    $options['row_border_color'] = array(
      'default' => '#cccccc',
    );

    $options['column_border_thickness'] = array(
      'default' => '1px',
    );
    $options['column_border_color'] = array(
      'default' => '#cccccc',
    );

    $options['frame_border_thickness'] = array(
      'default' => '1px',
    );
    $options['frame_border_color'] = array(
      'default' => '#cccccc',
    );
    return $options;
  }

  /**
   * Determine if a rendered field output should be treated as empty.
   *
   * @param mixed $value
   *   A rendered field value (usually a string, but may be an array).
   *
   * @return bool
   *   TRUE if the output is effectively empty.
   */
  protected function enhancedtable_is_effectively_empty($value) {
    // Preserve numeric zero.
    if ($value === 0 || $value === '0') {
      return FALSE;
    }

    if (is_array($value)) {
      $value = implode('', $value);
    }

    $value = (string) $value;
    // Treat markup-only whitespace as empty.
    $stripped = trim(strip_tags($value));
    return ($stripped === '');
  }

  public function render_fields($result) {
    // Render fields via Views core first.
    $rendered = parent::render_fields($result);

    // If we've already applied prefixes, return the cached/rendered output.
    if (!empty($this->enhancedtable_inline_titles_applied)) {
      return $rendered;
    }

    if (empty($rendered) || !is_array($rendered)) {
      return $rendered;
    }

    $inline = (!empty($this->options['inline_title']) && is_array($this->options['inline_title']))
      ? $this->options['inline_title']
      : array();
    if (empty($inline)) {
      return $rendered;
    }

    $merge_left = (!empty($this->options['merge_left']) && is_array($this->options['merge_left']))
      ? $this->options['merge_left']
      : array();

    if (empty($this->view) || empty($this->view->field) || !is_array($this->view->field)) {
      return $rendered;
    }

    // Walk fields left-to-right to determine merge groups.
    $root = NULL;
    foreach (array_keys($this->view->field) as $fid) {
      $want_merge = !empty($merge_left[$fid]);
      if (!$want_merge || !$root) {
        $root = $fid;
        continue;
      }

      if (empty($inline[$fid])) {
        continue;
      }

      $handler = isset($this->view->field[$fid]) ? $this->view->field[$fid] : NULL;
      if (!$handler) {
        continue;
      }

      // Use the field label as configured in the Views field settings.
      $label = '';
      if (method_exists($handler, 'label')) {
        $label = $handler->label();
      }
      elseif (!empty($handler->options['label'])) {
        $label = $handler->options['label'];
      }
      if ($label === '') {
        continue;
      }

      // Borrow punctuation from core Views field setting: "use colon after
      // label" (element_label_colon).
      $use_colon = TRUE;
      if (isset($handler->options) && is_array($handler->options) && array_key_exists('element_label_colon', $handler->options)) {
        $use_colon = !empty($handler->options['element_label_colon']);
      }

      $prefix = '<span class="enhancedtable-inline-title">' . check_plain($label) . '</span>';
      if ($use_colon) {
        $prefix .= '<span class="enhancedtable-inline-title-punct">:</span> ';
      }
      else {
        $prefix .= ' ';
      }

      foreach ($rendered as $row_index => &$row_fields) {
        if (!is_array($row_fields) || !array_key_exists($fid, $row_fields)) {
          continue;
        }

        // Avoid emitting a dangling label if the value is empty.
        if ($this->enhancedtable_is_effectively_empty($row_fields[$fid])) {
          continue;
        }

        $row_fields[$fid] = $prefix . $row_fields[$fid];
      }
      unset($row_fields);
    }

    // Keep Views' internal cache in sync.
    $this->rendered_fields = $rendered;

    // Mark as applied so subsequent render_fields() calls don't double-prefix.
    $this->enhancedtable_inline_titles_applied = TRUE;

    return $rendered;
  }


  /**
   * Apply EnhancedTable presentation order before rendering.
   *
   * This does not change the underlying Views query; it only influences the
   * order in which fields are rendered (and therefore the table column order).
   */
  public function pre_render($result) {
    // Match core signature: views_plugin_style::pre_render($result)
    parent::pre_render($result);

    if (empty($this->view) || empty($this->view->field) || empty($this->options['field_order'])) {
      return;
    }

    $weights = $this->options['field_order'];
    if (!is_array($weights) || empty($weights)) {
      return;
    }

    // Build the weighted list via foreach (also silences some static analyzers).
    $original = (array) $this->view->field;

    $weighted = array();
    $delta = 0;
    foreach ($original as $field_id => $handler) {
      $w = isset($weights[$field_id]) ? (int) $weights[$field_id] : ($delta * 10);
      $weighted[] = array(
        'field' => $field_id,
        'weight' => $w,
        'delta' => $delta,
      );
      $delta++;
    }

    // Avoid the spaceship operator for broader PHP compatibility.
    usort($weighted, function($a, $b) {
      if ($a['weight'] == $b['weight']) {
        if ($a['delta'] == $b['delta']) {
          return 0;
        }
        return ($a['delta'] < $b['delta']) ? -1 : 1;
      }
      return ($a['weight'] < $b['weight']) ? -1 : 1;
    });

    $reordered = array();
    foreach ($weighted as $item) {
      $fid = $item['field'];
      if (isset($original[$fid])) {
        $reordered[$fid] = $original[$fid];
      }
    }

    if (!empty($reordered)) {
      $this->view->field = $reordered;
    }
  }

  /**
   * Pass-through submit handler (keep for debugging if needed).
   */
  public function options_submit(&$form, &$form_state) {
    // Translate EnhancedTable's intuitive layout controls into the stock Views
    // table-style "columns" settings.
    if (!empty($form_state['values']['style_options']) && is_array($form_state['values']['style_options'])) {
      $so =& $form_state['values']['style_options'];

      // Determine field order from submitted weights.
      $order_weights = !empty($so['field_order']) && is_array($so['field_order']) ? $so['field_order'] : array();
      $order = array();

      if (!empty($order_weights)) {
        $tmp = array();
        foreach ($order_weights as $fid => $w) {
          $tmp[] = array('field' => $fid, 'weight' => (int) $w);
        }
        // Avoid the spaceship operator for broader PHP compatibility.
        usort($tmp, function($a, $b) {
          if ($a['weight'] == $b['weight']) {
            return strcmp($a['field'], $b['field']);
          }
          return ($a['weight'] < $b['weight']) ? -1 : 1;
        });
        foreach ($tmp as $item) {
          $order[] = $item['field'];
        }
      }

      // Fallback to view handler order if weights are missing.
      if (empty($order) && !empty($this->display) && !empty($this->display->handler)) {
        $handlers = $this->display->handler->get_handlers('field');
        if (!empty($handlers)) {
          $order = array_keys($handlers);
        }
      }

      $merge_left = !empty($so['merge_left']) && is_array($so['merge_left']) ? $so['merge_left'] : array();

      // Preserve existing mapping for fields that are not in our UI.
      $existing_columns = array();
      if (!empty($so['columns']) && is_array($so['columns'])) {
        $existing_columns = $so['columns'];
      }
      elseif (!empty($this->options['columns']) && is_array($this->options['columns'])) {
        $existing_columns = $this->options['columns'];
      }

      $new_columns = $existing_columns;

      // Compute merge-left.
      //
      // Stock Views table UI allows multiple fields to be placed into the same
      // column, but only the *parent column field* is a real column. Child
      // fields must be mapped directly to that parent.
      //
      // EnhancedTable's UI expresses this as "Merge left" on each field.
      // Therefore we map each merged field to the last non-merged field to its
      // left (the parent column).
      $root = NULL;
      foreach ($order as $fid) {
        $want_merge = !empty($merge_left[$fid]);
        if ($want_merge && $root) {
          $new_columns[$fid] = $root;
        }
        else {
          $new_columns[$fid] = $fid;
          $root = $fid;
        }
      }

      $so['columns'] = $new_columns;

      // Separator mapping.
      //
      // Stock Views table UI allows only one separator per combined column. It
      // is owned by the *parent column field* (the left-most field in that
      // column group).
      //
      // Older EnhancedTable versions collected the separator on the child being
      // merged.
      //
      // Copy (only the first child) separator onto its parent so Views uses it
      // at render-time.
      //
      // IMPORTANT: Do NOT delete child separators.
      // Views will ignore separators on non-parent fields when they are in the
      // same combined column, but keeping the values allows the user to unmerge
      // later and recover their previous separator settings.
      if (!empty($so['info']) && is_array($so['info'])) {
        $info =& $so['info'];
        $root = NULL;
        $first_child = TRUE;
        foreach ($order as $fid) {
          $want_merge = !empty($merge_left[$fid]);
          if ($want_merge && $root) {
            // Only the first merged child can contribute a separator.
            if ($first_child && array_key_exists('separator', $info[$fid])) {
              if (!isset($info[$root]) || !is_array($info[$root])) {
                $info[$root] = array();
              }
              // Always overwrite the parent separator with the first child's
              // submitted value (mirrors stock table behavior: only one
              // separator is used per combined column).
              $info[$root]['separator'] = $info[$fid]['separator'];
            }
            $first_child = FALSE;
          }
          else {
            // New parent column.
            $root = $fid;
            $first_child = TRUE;
          }
        }
      }

      // Click-sorting cleanup.
      // If a column is not sortable, ensure it cannot remain the default
      // click-sort column (which can leave an 'active' header class behind).
      $sortable_fields = array();
      if (!empty($so['info']) && is_array($so['info'])) {
        foreach ($so['info'] as $fid => &$info) {
          $is_sortable = !empty($info['sortable']);
          if ($is_sortable) {
            $sortable_fields[$fid] = TRUE;
          }
          else {
            // These options only make sense when sortable.
            if (isset($info['default_sort_order'])) {
              unset($info['default_sort_order']);
            }
          }
        }
        unset($info);
      }

      // If the chosen default is no longer sortable, fall back to "no default".
      if (isset($so['default'])) {
        $def = $so['default'];
        if ($def !== -1 && $def !== '-1' && $def !== '' && empty($sortable_fields[$def])) {
          $so['default'] = -1;
        }
        // If nothing is sortable, force no default.
        if (empty($sortable_fields)) {
          $so['default'] = -1;
        }
      }
    }

    parent::options_submit($form, $form_state);
    // Uncomment while debugging if you want to inspect saved options:
    // watchdog('enhancedtable', 'opts:<pre>@o</pre>', ['@o' => print_r($this->options, TRUE)], WATCHDOG_DEBUG);
  }

  // TEMPORARILY DISABLE THE PREVIEW

  
}
