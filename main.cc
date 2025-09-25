// app can create and delete 2d lines, import svg files and pdfexport and print, work on Debian 13 and Windows
// for create Latex svg is nessesary have installed: 
//  pacman -S mingw-w64-ucrt-x86_64-texlive-full
//  pdflatex.exe is nessesary copy from MikTex instalation to c:\msys64\mingw64\bin\ folder
//  pacman -S mingw-w64-x86_64-pdf2svg
//compile:
//g++ -std=c++17 main.cc -o app $(pkg-config --cflags --libs gtkmm-4.0 librsvg-2.0)
//with icons:
//g++ -std=c++17 main.cc resource.c -o app $(pkg-config --cflags --libs gtkmm-4.0 librsvg-2.0)


#include <sigc++/sigc++.h> // Ensure this is included early
#include <gtkmm/application.h>
#include <iostream>
#include <gtkmm.h>
#include <cairomm/context.h>
#include <cairomm/cairomm.h>
#include <fstream>
#include <memory> 
#include <gdk/gdkkeysyms.h>
#include <map>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include <string>
#include <giomm/menu.h>
#include <giomm/menuitem.h>
#include <glibmm/variant.h>
#include <valarray>
#include <iomanip>
#include <sstream>
#include <gdkmm/event.h>
#include <gdkmm/display.h>
#include <giomm/file.h>
#include <gtkmm/filechoosernative.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackpage.h>
#include <gtkmm/stackswitcher.h>
#include <clocale> // Pro std::setlocale
#include <locale>  // Pro std::locale
#include <glibmm/i18n.h> // Pro Glib::setenv a bindtextdomain
#include <fstream>
#include <string>
#include <librsvg/rsvg.h> // Include librsvg for SVG handling
#include <cstdio> // For std::remove

// LaTeX to SVG conversion function (from user's provided code)
int latex2svg(const std::string& equ, const std::string& output_filename_base)
{
    std::string tex_filename = output_filename_base + ".tex";
    std::string pdf_filename = output_filename_base + ".pdf";
    std::string svg_filename = output_filename_base + ".svg";
    std::string log_filename = output_filename_base + ".log";
    std::string aux_filename = output_filename_base + ".aux";


   std::ofstream myfile;
    myfile.open(tex_filename);
    myfile << "\\documentclass[class=article,border=2pt]{standalone}" << std::endl;
    myfile << "\\usepackage{amsmath}" << std::endl; 
    myfile << "\\usepackage{amssymb}" << std::endl;
    myfile << "\\usepackage[utf8]{inputenc}" << std::endl;
    myfile << "\\usepackage[T1]{fontenc}" << std::endl; 
    myfile << "\\usepackage[czech]{babel}" << std::endl;
    myfile << "\\begin{document}" << std::endl;
    myfile << "\\noindent\\begin{minipage}{20cm}" << std::endl; 
    myfile << equ << std::endl;                               
    myfile << "\\end{minipage}" << std::endl;
    myfile << "\\end{document}" << std::endl;
    myfile.close();

    // Run pdflatex without redirecting stdout/stderr to capture log file
    std::string command = "pdflatex -interaction=nonstopmode " + tex_filename;
    int retC = system(command.c_str());

    if (retC != 0) {
        std::cerr << "Error running pdflatex for: " + tex_filename << std::endl;
        // Attempt to read and print the log file for more details
        std::ifstream log_file(log_filename);
        if (log_file.is_open()) {
            std::string line;
            std::cerr << "--- pdflatex log for " << tex_filename << " ---" << std::endl;
            while (std::getline(log_file, line)) {
                std::cerr << line << std::endl;
            }
            std::cerr << "--- End of log ---" << std::endl;
            log_file.close();
        } else {
            std::cerr << "Could not open pdflatex log file: " << log_filename << std::endl;
        }
        // Clean up temporary files, including the .log file
        std::remove(tex_filename.c_str());
        std::remove(pdf_filename.c_str()); // PDF might have been created partially
        std::remove(log_filename.c_str());
        std::remove(aux_filename.c_str());
        return retC;
    }

    command = "pdf2svg " + pdf_filename + " " + svg_filename + " > /dev/null 2>&1";
    retC = system(command.c_str());

    if (retC != 0) {
        std::cerr << "Error running pdf2svg for: " + pdf_filename << std::endl;
    }

    // Clean up temporary files
    std::remove(pdf_filename.c_str());
    std::remove(log_filename.c_str());
    std::remove(aux_filename.c_str());
    std::remove(tex_filename.c_str());

    return retC;
}


enum class ObjectType {
    NONE,
    POINT,
    LINE,
    ARC,
    SVG // New object type for SVG
};

std::string objectTypeToString(ObjectType type) {
    static const std::map<ObjectType, std::string> typeMap = {
        {ObjectType::NONE, "NONE"},
        {ObjectType::POINT, "POINT"},
        {ObjectType::LINE, "LINE"},
        {ObjectType::ARC, "ARC"},
        {ObjectType::SVG, "SVG"} // Add SVG to the map
    };
    auto it = typeMap.find(type);
    if (it != typeMap.end()) {
        return it->second;
    }
    return "Unknown";
}

ObjectType stringToObjectType(const std::string& str) {
    static const std::map<std::string, ObjectType> stringMap = {
        {"NONE", ObjectType::NONE},
        {"POINT", ObjectType::POINT},
        {"LINE", ObjectType::LINE},
        {"ARC", ObjectType::ARC},
        {"SVG", ObjectType::SVG} // Add SVG to the map
    };
    auto it = stringMap.find(str);
    if (it != stringMap.end()) {
        return it->second;
    }
    return ObjectType::NONE;
}

class Point2D : public Glib::Object {
public:
    double x, y;

    Point2D(double px = 0.0, double py = 0.0) : x(px), y(py) {}

    static Glib::RefPtr<Point2D> create(double px = 0.0, double py = 0.0) {
        return Glib::make_refptr_for_instance(new Point2D(px, py));
    }
};

class Line : public Glib::Object {
public:
    Glib::RefPtr<Point2D> start_point;
    Glib::RefPtr<Point2D> end_point;
    double thickness;
    std::valarray<double> style_pattern;
    double color_r, color_g, color_b;
    bool m_is_selected;

    Line(Glib::RefPtr<Point2D> p1, Glib::RefPtr<Point2D> p2,
         double t, const std::valarray<double>& sp, double r, double g, double b)
        : start_point(p1), end_point(p2), thickness(t), style_pattern(sp),
          color_r(r), color_g(g), color_b(b), m_is_selected(false) {}

    static Glib::RefPtr<Line> create(Glib::RefPtr<Point2D> p1, Glib::RefPtr<Point2D> p2,
                                     double t, const std::valarray<double>& sp, double r, double g, double b) {
        return Glib::make_refptr_for_instance(new Line(p1, p2, t, sp, r, g, b));
    }

    std::string get_style_name() const {
        if (style_pattern.size() == 0) return "Solid";
        if (style_pattern.size() == 2 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3) return "Dashed";
        if (style_pattern.size() == 4 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3 && style_pattern[2] == .3 && style_pattern[3] == 1.3) return "Dash-dot";
        if (style_pattern.size() == 6 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3 && style_pattern[2] == .3 && style_pattern[3] == 1.3 && style_pattern[4] == .3 && style_pattern[5] == 1.3) return "Dash-double-dot";
        return "Custom";
    }

    std::string get_color_name() const {
        if (color_r == 0.0 && color_g == 0.0 && color_b == 0.0) return "Black";
        if (color_r == 1.0 && color_g == 0.0 && color_b == 0.0) return "Red";
        if (color_r == 0.0 && color_g == 0.0 && color_b == 1.0) return "Blue";
        if (color_r == 0.0 && color_g == 1.0 && color_b == 0.0) return "Green";
        return "R:" + std::to_string(static_cast<int>(color_r * 255)) + " G:" + std::to_string(static_cast<int>(color_g * 255)) + " B:" + std::to_string(static_cast<int>(color_b * 255));
    }
};

// New class for SVG objects
class SvgObject : public Glib::Object {
public:
    RsvgHandle* m_svg_handle;
    Glib::RefPtr<Point2D> m_position; // Position of the SVG object (top-left corner)
    double m_scale_factor; // Scale factor for the SVG
    bool m_flip_horizontal; // NEW: Flag to indicate if the SVG should be flipped horizontally

    SvgObject(RsvgHandle* handle, Glib::RefPtr<Point2D> pos, double scale = 1.0, bool flip_h = false) 
        : m_svg_handle(handle), m_position(pos), m_scale_factor(scale), m_flip_horizontal(flip_h) { 
        if (m_svg_handle) {
            g_object_ref(m_svg_handle); // Increment reference count
        }
    }

    ~SvgObject() override {
        if (m_svg_handle) {
            g_object_unref(m_svg_handle); // Decrement reference count
        }
    }

    static Glib::RefPtr<SvgObject> create(RsvgHandle* handle, Glib::RefPtr<Point2D> pos, double scale = 1.0, bool flip_h = false) { 
        return Glib::make_refptr_for_instance(new SvgObject(handle, pos, scale, flip_h)); 
    }
};


class TreeColumns : public Gtk::TreeModel::ColumnRecord {
public:
    TreeColumns() {
        add(m_col_id);
        add(m_col_name);
        add(m_col_object_type);
        add(m_col_generic_object);
        add(m_col_thickness);
        add(m_col_style_name);
        add(m_col_color_name);
    }

    Gtk::TreeModelColumn<int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    Gtk::TreeModelColumn<ObjectType> m_col_object_type;
    Gtk::TreeModelColumn<Glib::RefPtr<Glib::Object>> m_col_generic_object;
    Gtk::TreeModelColumn<double> m_col_thickness;
    Gtk::TreeModelColumn<Glib::ustring> m_col_style_name;
    Gtk::TreeModelColumn<Glib::ustring> m_col_color_name;
};

class TreeList: public Gtk::ScrolledWindow
{
public:
  TreeList();
  ~TreeList() override;
private:
  Glib::RefPtr<Gtk::CssProvider> m_refCssProvider;

protected:
  void on_setup_message(const Glib::RefPtr<Gtk::ListItem>& list_item);
  void on_bind_message(const Glib::RefPtr<Gtk::ListItem>& list_item);
  static void on_parsing_error(const Glib::RefPtr<const Gtk::CssSection>& section,
    const Glib::Error& error);

  Glib::RefPtr<Gtk::StringList> m_refStringList;

public:
  Gtk::TreeView m_TreeView;
  TreeColumns m_Columns;

   Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;

   Glib::RefPtr<Gtk::TreeSelection> get_selection();
};


Glib::RefPtr<Gtk::TreeSelection> TreeList::get_selection()
{
    return m_TreeView.get_selection();
}

TreeList::TreeList()
{
  set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  set_child(m_TreeView);
  m_refTreeModel = Gtk::TreeStore::create(m_Columns);
  m_TreeView.set_model(m_refTreeModel);
  m_TreeView.append_column("Name", m_Columns.m_col_name);
  m_TreeView.append_column("Thickness", m_Columns.m_col_thickness);
  m_TreeView.append_column("Style", m_Columns.m_col_style_name);
  m_TreeView.append_column("Color", m_Columns.m_col_color_name);

  m_TreeView.get_selection()->set_mode(Gtk::SelectionMode::MULTIPLE);

  m_TreeView.set_show_expanders(true);
  m_TreeView.set_enable_tree_lines(true);

  m_refCssProvider = Gtk::CssProvider::create();
  m_refCssProvider->load_from_data(
      "treeview.view.expander { -gtk-icon-source: -gtk-icontheme(\"zoom-in-symbolic\"); color: #4d4d4d; }\n"
      "treeview.view.expander:dir(rtl) { -gtk-icon-source: -gtk-icontheme(\"pan-end-symbolic-rtl\"); }\n"
      "treeview.view.expander:checked { -gtk-icon-source: -gtk-icontheme(\"zoom-out-symbolic\"); }\n"
  );
  m_TreeView.get_style_context()->add_provider(m_refCssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

TreeList::~TreeList()
{
}

void TreeList::on_setup_message(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  auto label = Gtk::make_managed<Gtk::Label>();
  label->set_halign(Gtk::Align::START);
  list_item->set_child(*label);
}

void TreeList::on_bind_message(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
  auto pos = list_item->get_position();
  if (pos == GTK_INVALID_LIST_POSITION)
    return;
  auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
  if (!label)
    return;
  label->set_text(m_refStringList->get_string(pos));
}


class DrwArea : public Gtk::DrawingArea
{
public:
  DrwArea();
  virtual ~DrwArea();
  void set_tree_store(const Glib::RefPtr<Gtk::TreeStore>& tree_store, const TreeColumns& columns, Gtk::TreeView* tree_view);
  void redraw_all();
  void set_line_thickness(double thickness);
  void set_line_style(const std::valarray<double>& style);
  void set_line_color(double r, double g, double b);

  double get_current_line_thickness() const { return m_current_line_thickness; }
  const std::valarray<double>& get_current_line_style() const { return m_current_line_style; }
  double get_current_line_color_r() const { return m_current_line_color_r; }
  double get_current_line_color_g() const { return m_current_line_color_g; }
  double get_current_line_color_b() const { return m_current_line_color_b; }

  void clear_selection_and_highlight();

  void draw_content_to_cairo_context(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

  // New methods for SVG placement
  void start_svg_placement(const Glib::ustring& filename); // Modified: Removed scale_factor and flip_horizontal
  void set_current_svg_placement_scale(double scale) { m_current_svg_placement_scale = scale; queue_draw(); } // New setter
  void set_current_svg_placement_flip_horizontal(bool flip) { m_current_svg_placement_flip_horizontal = flip; queue_draw(); } // New setter
  bool get_is_placing_svg() const { return m_is_placing_svg; } // New getter

private:
  // Private helper for adding SVG at specific coordinates
  void add_svg_object_at_coords(const Glib::ustring& filename, double click_x, double click_y);

  double m_mouse_x;
  double m_mouse_y;
  double m_last_mouse_x;
  double m_last_mouse_y;

  double m_scale;
  double m_center_x_drawing;
  double m_center_y_drawing;

  bool m_pan_on;
  Gtk::PopoverMenu m_MenuPopup;

  Glib::RefPtr<Point2D> m_line_start_point;
  Glib::RefPtr<Point2D> m_line_current_mouse_pos;
  bool m_is_drawing_line;
  bool m_first_click_for_line;

  bool m_is_placing_svg; 
  Glib::ustring m_svg_file_to_place; 
  double m_current_svg_placement_scale; 
  bool m_current_svg_placement_flip_horizontal; 

  RsvgHandle* m_preview_svg_handle; 
  Glib::ustring m_preview_svg_filename; 

  Glib::RefPtr<Gtk::EventControllerKey> m_key_controller;
  Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;
  const TreeColumns* m_Columns_ptr = nullptr;
  int m_next_object_id;
  Gtk::TreeView* m_tree_view_ptr = nullptr;

  Glib::RefPtr<Point2D> m_highlighted_point;
  Glib::RefPtr<Line> m_highlighted_line;
  double m_current_line_thickness;
  std::valarray<double> m_current_line_style;
  double m_current_line_color_r, m_current_line_color_g, m_current_line_color_b;
  bool m_shift_is_pressed;

public:
  static const std::valarray<double> DASHED_PATTERN;
  static const std::valarray<double> DASHDOTED_PATTERN;
  static const std::valarray<double> DASHDDOTTED_PATTERN;

  // Made public for App2d to access for initial zoom
  void zoom_to_a4();
  // Public setters for scale and center coordinates
  void set_scale(double scale);
  void set_center_x(double x);
  void set_center_y(double y);
  // Public getters for scale and center coordinates
  double get_scale() const { return m_scale; }
  double get_center_x_drawing() const { return m_center_x_drawing; }
  double get_center_y_drawing() const { return m_center_y_drawing; }


protected:
  void on_popup_button_pressed(int , double x, double y);
  void on_menu_file_popup_generic();
  void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
  void on_midb_pre(int n_press, double x, double y);
  void on_midb_rel(int n_press, double x, double y);
  void on_mmotion(double x, double y);
  bool on_mscr(double dx, double dy);
  void on_primary_mouse_click(int n_press, double x, double y);

  Glib::RefPtr<Point2D> screen_to_drawing_coords(double screen_x, double screen_y);
  Glib::RefPtr<Point2D> drawing_to_screen_coords(double drawing_x, double drawing_y);
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void on_key_released(guint keyval, guint keycode, Gdk::ModifierType state);
  void on_middle_button_double_click(int n_press, double x, double y);

  Glib::RefPtr<Point2D> find_closest_point(double mouse_x, double mouse_y, double tolerance_pixels);
  Glib::RefPtr<Line> find_closest_line(double mouse_x, double mouse_y, double tolerance_pixels);
  void zoom_to_all_points();


public:
  bool toggle_line_drawing_mode();
  sigc::signal<void(bool)> signal_line_drawing_mode_changed;
  sigc::signal<void(double, double)> signal_cursor_moved;
  sigc::signal<void(bool)> signal_svg_placement_mode_changed; 
  bool get_first_click_for_line() const { return m_first_click_for_line; }
  void set_first_click_for_line(bool value) { m_first_click_for_line = value; }
  Glib::RefPtr<Point2D> get_line_start_point() const { return m_line_start_point; }
  void set_line_start_point(Glib::RefPtr<Point2D> p) { m_line_start_point = p; }
  Glib::RefPtr<Point2D> get_line_current_mouse_pos() const { return m_line_current_mouse_pos; }
  void set_line_current_mouse_pos(Glib::RefPtr<Point2D> p) { m_line_current_mouse_pos = p; }

  Glib::RefPtr<Gtk::TreeStore> get_tree_model() const { return m_refTreeModel; }
  const TreeColumns* get_columns_ptr() const { return m_Columns_ptr; }
  int& get_next_object_id() { return m_next_object_id; }
  Gtk::TreeView* get_tree_view_ptr() const { return m_tree_view_ptr; }

  std::vector<Glib::RefPtr<Line>> m_selected_lines;
};

const std::valarray<double> DrwArea::DASHED_PATTERN = {3.3, 1.3};
const std::valarray<double> DrwArea::DASHDOTED_PATTERN = {3.3, 1.3, .3, 1.3};
const std::valarray<double> DrwArea::DASHDDOTTED_PATTERN = {3.3, 1.3, .3, 1.3, .3, 1.3};


Glib::RefPtr<Point2D> DrwArea::screen_to_drawing_coords(double screen_x, double screen_y)
{
    double drawing_x = (screen_x - get_width() / 2.0) / m_scale + m_center_x_drawing;
    double drawing_y = -(screen_y - get_height() / 2.0) / m_scale + m_center_y_drawing;
    return Point2D::create(drawing_x, drawing_y);
}

Glib::RefPtr<Point2D> DrwArea::drawing_to_screen_coords(double drawing_x, double drawing_y)
{
    double screen_x = (drawing_x - m_center_x_drawing) * m_scale + get_width() / 2.0;
    double screen_y = (drawing_y - m_center_y_drawing) * -m_scale + get_height() / 2.0;
    return Point2D::create(screen_x, screen_y);
}


DrwArea::DrwArea()
{
  set_draw_func(sigc::mem_fun(*this, &DrwArea::on_draw));
  m_scale = 1.7; // Initial scale, can be changed via settings
  m_center_x_drawing = 100.0; // Initial center X, can be changed via settings
  m_center_y_drawing = 150.0; // Initial center Y, can be changed via settings

  m_mouse_x = 0.0;
  m_mouse_y = 0.0;
  m_last_mouse_x = 0.0;
  m_last_mouse_y = 0.0;
  m_pan_on = false;

  m_is_drawing_line = false;
  m_first_click_for_line = false;
  m_is_placing_svg = false; // Initialize new flag
  m_svg_file_to_place = "";
  m_current_svg_placement_scale = 1.0; // Initialize new scale factor
  m_current_svg_placement_flip_horizontal = false; 
  m_preview_svg_handle = nullptr; // Initialize new preview handle
  m_preview_svg_filename = "";

  m_next_object_id = 1;
  m_highlighted_point = nullptr;
  m_highlighted_line = nullptr;
  m_selected_lines.clear();
  m_current_line_thickness = 1.0;
  m_current_line_style = {};
  m_current_line_color_r = 0.0;
  m_current_line_color_g = 0.0;
  m_current_line_color_b = 0.0;
  m_shift_is_pressed = false;

  auto mouse_scroll = Gtk::EventControllerScroll::create();
  mouse_scroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    mouse_scroll->signal_scroll().connect(
    sigc::mem_fun(*this, &DrwArea::on_mscr), true);
  add_controller(mouse_scroll);;

  auto mouse_click = Gtk::GestureClick::create();
  mouse_click->set_button(GDK_BUTTON_MIDDLE);
  mouse_click->signal_released().connect(sigc::mem_fun(*this, &DrwArea::on_midb_rel));
  add_controller(mouse_click);

  auto m_second_button_click = Gtk::GestureClick::create();
  m_second_button_click->set_button(GDK_BUTTON_MIDDLE);
  m_second_button_click->signal_pressed().connect(sigc::mem_fun(*this, &DrwArea::on_midb_pre));
 add_controller(m_second_button_click);

    auto middle_double_click = Gtk::GestureClick::create();
    middle_double_click->set_button(GDK_BUTTON_MIDDLE);

    middle_double_click->signal_pressed().connect(
        sigc::mem_fun(*this, &DrwArea::on_middle_button_double_click)
    );
    add_controller(middle_double_click);


    auto motion_controller = Gtk::EventControllerMotion::create();
  motion_controller->signal_motion().connect(
    sigc::mem_fun(*this, &DrwArea::on_mmotion));
  add_controller(motion_controller);

  auto refGesture = Gtk::GestureClick::create();
  refGesture->set_button(GDK_BUTTON_SECONDARY);
  refGesture->signal_pressed().connect(
    sigc::mem_fun(*this, &DrwArea::on_popup_button_pressed));
  add_controller(refGesture);

  auto gmenu = Gio::Menu::create();
  gmenu->append("Edit", "popup.edit");
  gmenu->append("Process", "popup.process");
  gmenu->append("Remove", "popup.remove");

  m_MenuPopup.set_parent(*this);
  m_MenuPopup.set_menu_model(gmenu);
  m_MenuPopup.set_has_arrow(false);

  auto refActionGroup = Gio::SimpleActionGroup::create();

  refActionGroup->add_action("edit",
    sigc::mem_fun(*this, &DrwArea::on_menu_file_popup_generic));
  refActionGroup->add_action("process",
    sigc::mem_fun(*this, &DrwArea::on_menu_file_popup_generic));
  refActionGroup->add_action("remove",
    sigc::mem_fun(*this, &DrwArea::on_menu_file_popup_generic));

  insert_action_group("popup", refActionGroup);

  auto gesture_click_line = Gtk::GestureClick::create();
  gesture_click_line->set_button(GDK_BUTTON_PRIMARY);
  gesture_click_line->signal_pressed().connect(
    sigc::mem_fun(*this, &DrwArea::on_primary_mouse_click));
  add_controller(gesture_click_line);

  m_key_controller = Gtk::EventControllerKey::create();
  add_controller(m_key_controller);
  m_key_controller->signal_key_pressed().connect(
    sigc::mem_fun(*this, &DrwArea::on_key_pressed), false);

  m_key_controller->signal_key_released().connect(
    [this](guint keyval, guint keycode, Gdk::ModifierType state) {
      this->on_key_released(keyval, keycode, state);
    }, false
  );
}

DrwArea::~DrwArea()
{
    // Clean up the preview SVG handle if it's still holding an object
    if (m_preview_svg_handle) {
        g_object_unref(m_preview_svg_handle);
        m_preview_svg_handle = nullptr;
    }
}

void DrwArea::set_tree_store(const Glib::RefPtr<Gtk::TreeStore>& tree_store, const TreeColumns& columns, Gtk::TreeView* tree_view)
{
    m_refTreeModel = tree_store;
    m_Columns_ptr = &columns;
    m_tree_view_ptr = tree_view;
}

void DrwArea::redraw_all()
{
    queue_draw();
}

void DrwArea::set_line_thickness(double thickness)
{
    m_current_line_thickness = thickness;
    queue_draw();
}

void DrwArea::set_line_style(const std::valarray<double>& style)
{
    m_current_line_style = style;
    queue_draw();
}

void DrwArea::set_line_color(double r, double g, double b)
{
    m_current_line_color_r = r;
    m_current_line_color_g = g;
    m_current_line_color_b = b;
    queue_draw();
}

void DrwArea::on_popup_button_pressed(int , double x, double y)
{
  const Gdk::Rectangle rect(x, y, 1, 1);
  m_MenuPopup.set_pointing_to(rect);
  m_MenuPopup.popup();
}

void DrwArea::on_menu_file_popup_generic()
{
  std::cout << "Popup menu item selected." << std::endl;
}

bool DrwArea::on_mscr(double dx, double dy)
{
    std::cout << "Mouse scroll: "  << dx << ", " << dy << std::endl;

    double old_scale = m_scale;

    double mouse_screen_x_relative = m_mouse_x - get_width() / 2.0;
    double mouse_screen_y_relative = m_mouse_y - get_height() / 2.0;

    double mouse_drawing_x = mouse_screen_x_relative / old_scale + m_center_x_drawing;
    double mouse_drawing_y = -mouse_screen_y_relative / old_scale + m_center_y_drawing;

    double zoom_increment = dy / 3.0;
    m_scale += zoom_increment;

    const double MIN_SCALE = 0.01;
    const double MAX_SCALE = 100.0;

    if (m_scale < MIN_SCALE) {
        m_scale = MIN_SCALE;
    }
    if (m_scale > MAX_SCALE) {
        m_scale = MAX_SCALE;
    }

    if (m_scale != old_scale) {
        m_center_x_drawing = mouse_drawing_x - mouse_screen_x_relative / m_scale;
        m_center_y_drawing = mouse_drawing_y + mouse_screen_y_relative / m_scale;
    }

    std::cout << "Scale: "  << m_scale << ", Center: " << m_center_x_drawing << ", " << m_center_y_drawing << std::endl;
    queue_draw();

    return true;
}

void DrwArea::clear_selection_and_highlight()
{
    for (const auto& line_obj : m_selected_lines) {
        line_obj->m_is_selected = false;
    }
    m_selected_lines.clear();
    m_highlighted_line = nullptr;
    m_highlighted_point = nullptr;
    if (m_tree_view_ptr) {
        m_tree_view_ptr->get_selection()->unselect_all();
    }
    queue_draw();
}

bool DrwArea::toggle_line_drawing_mode()
{
  m_is_drawing_line = !m_is_drawing_line;
  m_first_click_for_line = false;
  if (!m_is_drawing_line) {
      clear_selection_and_highlight();
  }
  // If we toggle line drawing, cancel SVG placement
  if (m_is_placing_svg) {
      m_is_placing_svg = false;
      m_svg_file_to_place = "";
      m_current_svg_placement_scale = 1.0; // Reset scale
      m_current_svg_placement_flip_horizontal = false; // Reset flip
      if (m_preview_svg_handle) { // Clean up preview handle
          g_object_unref(m_preview_svg_handle);
          m_preview_svg_handle = nullptr;
      }
      signal_svg_placement_mode_changed.emit(false);
  }
  std::cout << "Line drawing mode: " << (m_is_drawing_line ? "ON" : "OFF") << std::endl;
  queue_draw();
  signal_line_drawing_mode_changed.emit(m_is_drawing_line);
  return m_is_drawing_line;
}

void DrwArea::on_primary_mouse_click(int , double x, double y)
{
    bool shift_pressed = m_shift_is_pressed;

    if (m_is_placing_svg) {
        add_svg_object_at_coords(m_svg_file_to_place, x, y);
        m_is_placing_svg = false;
        m_svg_file_to_place = "";
        m_current_svg_placement_scale = 1.0; // Reset scale after placement
        m_current_svg_placement_flip_horizontal = false; // Reset flip after placement
        if (m_preview_svg_handle) { // Clean up preview handle
            g_object_unref(m_preview_svg_handle);
            m_preview_svg_handle = nullptr;
        }
        signal_svg_placement_mode_changed.emit(false);
        std::cout << "SVG placed at click location." << std::endl;
        queue_draw();
        return; // Consume the click
    }


    if (m_is_drawing_line) {
        Glib::RefPtr<Point2D> click_point;

        if (m_highlighted_point) {
            click_point = m_highlighted_point;
            std::cout << "Using highlighted point: (" << click_point->x << ", " << click_point->y << ")" << std::endl;
        } else {
            click_point = screen_to_drawing_coords(x, y);
        }

        if (!m_first_click_for_line) {
            m_line_start_point = click_point;
            m_line_current_mouse_pos = click_point;
            m_first_click_for_line = true;
            std::cout << "first click"  << std::endl;
        } else {
            std::cout << "second click"  << std::endl;
            Glib::RefPtr<Line> new_line_obj = Line::create(
                m_line_start_point, click_point,
                m_current_line_thickness, m_current_line_style,
                m_current_line_color_r, m_current_line_color_g, m_current_line_color_b
            );

            if (m_refTreeModel && m_Columns_ptr && m_tree_view_ptr) {
                Gtk::TreeModel::iterator parent_line_it = m_refTreeModel->append();
                auto row = *parent_line_it;

                row[m_Columns_ptr->m_col_id] = m_next_object_id++;
                row[m_Columns_ptr->m_col_name] = "Line";
                row[m_Columns_ptr->m_col_object_type] = ObjectType::LINE;
                row[m_Columns_ptr->m_col_generic_object] = new_line_obj;
                row[m_Columns_ptr->m_col_thickness] = new_line_obj->thickness;
                row[m_Columns_ptr->m_col_style_name] = new_line_obj->get_style_name();
                row[m_Columns_ptr->m_col_color_name] = new_line_obj->get_color_name();

                std::cout << "Line added: ID=" << row[m_Columns_ptr->m_col_id] << ", Name=" << row[m_Columns_ptr->m_col_name]
                          << ", Type=" << objectTypeToString(row[m_Columns_ptr->m_col_object_type])
                          << ", Thickness=" << row[m_Columns_ptr->m_col_thickness]
                          << ", Style=" << row[m_Columns_ptr->m_col_style_name]
                          << ", Color=" << row[m_Columns_ptr->m_col_color_name] << std::endl;

                std::stringstream ss_start, ss_end, ss_mid;
                ss_start << std::fixed << std::setprecision(2) << m_line_start_point->x << ", " << m_line_start_point->y;
                ss_end << std::fixed << std::setprecision(2) << click_point->x << ", " << click_point->y;

                Gtk::TreeModel::iterator child_it_start = m_refTreeModel->append(row.children());
                auto child_row_start = *child_it_start;
                child_row_start[m_Columns_ptr->m_col_id] = m_next_object_id++;
                child_row_start[m_Columns_ptr->m_col_name] = "Start Point (" + ss_start.str() + ")";
                child_row_start[m_Columns_ptr->m_col_object_type] = ObjectType::POINT;
                child_row_start[m_Columns_ptr->m_col_generic_object] = m_line_start_point;
                std::cout << "  - Start point added: ID=" << child_row_start[m_Columns_ptr->m_col_id] << std::endl;

                Gtk::TreeModel::iterator child_it_end = m_refTreeModel->append(row.children());
                auto child_row_end = *child_it_end;
                child_row_end[m_Columns_ptr->m_col_name] = "End Point (" + ss_end.str() + ")";
                child_row_end[m_Columns_ptr->m_col_object_type] = ObjectType::POINT;
                child_row_end[m_Columns_ptr->m_col_generic_object] = click_point;
                std::cout << "  - End point added: ID=" << child_row_end[m_Columns_ptr->m_col_id] << std::endl;

                double mid_x = (m_line_start_point->x + click_point->x) / 2.0;
                double mid_y = (m_line_start_point->y + click_point->y) / 2.0;
                Glib::RefPtr<Point2D> mid_point = Point2D::create(mid_x, mid_y);

                ss_mid << std::fixed << std::setprecision(2) << mid_x << ", " << mid_y;

                Gtk::TreeModel::iterator child_it_mid = m_refTreeModel->append(row.children());
                auto child_row_mid = *child_it_mid;
                child_row_mid[m_Columns_ptr->m_col_id] = m_next_object_id++;
                child_row_mid[m_Columns_ptr->m_col_name] = "Mid Point (" + ss_mid.str() + ")";
                child_row_mid[m_Columns_ptr->m_col_object_type] = ObjectType::POINT;
                child_row_mid[m_Columns_ptr->m_col_generic_object] = mid_point;
                std::cout << "  - Midpoint added: ID=" << child_row_mid[m_Columns_ptr->m_col_id] << std::endl;

                m_tree_view_ptr->expand_row(m_refTreeModel->get_path(parent_line_it), false);
            }

            m_first_click_for_line = false;
            m_highlighted_point = nullptr;
        } 
    } 
    else { 
        Glib::RefPtr<Line> clicked_line = find_closest_line(x, y, 5.0);

        if (m_tree_view_ptr && m_refTreeModel && m_Columns_ptr) {
            auto selection = m_tree_view_ptr->get_selection();

            if (shift_pressed) {
                if (clicked_line) {
                    Gtk::TreeModel::Children children = m_refTreeModel->children();
                    for (auto it = children.begin(); it != children.end(); ++it) {
                        ObjectType obj_type = (*it)[m_Columns_ptr->m_col_object_type];
                        if (obj_type == ObjectType::LINE) {
                            Glib::RefPtr<Glib::Object> generic_obj = (*it)[m_Columns_ptr->m_col_generic_object];
                            Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                            if (line_obj == clicked_line) {
                                if (line_obj->m_is_selected) {
                                    line_obj->m_is_selected = false;
                                    selection->unselect(m_refTreeModel->get_path(it));
                                    auto it_vec = std::remove(m_selected_lines.begin(), m_selected_lines.end(), line_obj);
                                    m_selected_lines.erase(it_vec, m_selected_lines.end());
                                    std::cout << "Line deselected (Shift): " << line_obj->get_color_name() << " " << line_obj->get_style_name() << std::endl;
                                } else {
                                    line_obj->m_is_selected = true;
                                    selection->select(m_refTreeModel->get_path(it));
                                    m_selected_lines.push_back(line_obj);
                                    std::cout << "Line selected (Shift): " << line_obj->get_color_name() << " " << line_obj->get_style_name() << std::endl;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    std::cout << "Shift click on empty space. Selection unchanged." << std::endl;
                }

            } else {
                if (clicked_line) {
                    Gtk::TreeModel::Children children = m_refTreeModel->children();
                    for (auto it = children.begin(); it != children.end(); ++it) {
                        ObjectType obj_type = (*it)[m_Columns_ptr->m_col_object_type];
                        if (obj_type == ObjectType::LINE) {
                            Glib::RefPtr<Glib::Object> generic_obj = (*it)[m_Columns_ptr->m_col_generic_object];
                            Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                            if (line_obj == clicked_line) {
                                if (m_selected_lines.size() == 1 && m_selected_lines[0] == line_obj) {
                                    clear_selection_and_highlight();
                                    std::cout << "Line deselected (Single Click, was only selected): " << line_obj->get_color_name() << " " << line_obj->get_style_name() << std::endl;
                                } else {
                                    clear_selection_and_highlight();
                                    line_obj->m_is_selected = true;
                                    selection->select(m_refTreeModel->get_path(it));
                                    m_selected_lines.push_back(line_obj);
                                    std::cout << "Line selected (Single Click, cleared others): " << line_obj->get_color_name() << " " << line_obj->get_style_name() << std::endl;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    clear_selection_and_highlight();
                    std::cout << "No line clicked. All deselected." << std::endl;
                }
            }
        }
        m_highlighted_line = nullptr;

        std::cout << "--- Selection State After Click ---" << std::endl;
        std::cout << "m_selected_lines size: " << m_selected_lines.size() << std::endl;
        for (const auto& selected_line : m_selected_lines) {
            std::cout << "  - Line in m_selected_lines (ptr: " << selected_line.get() << "): "
                      << selected_line->get_color_name() << " " << selected_line->get_style_name()
                      << " (m_is_selected: " << selected_line->m_is_selected << ")" << std::endl;
        }
        if (clicked_line) {
            std::cout << "Clicked line (ptr: " << clicked_line.get() << ") m_is_selected: " << clicked_line->m_is_selected << std::endl;
        }
        std::cout << "-----------------------------------" << std::endl;
    }
    queue_draw();
}

bool DrwArea::on_key_pressed(guint keyval, guint , Gdk::ModifierType state)
{
    if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R) {
        m_shift_is_pressed = true;
    }

    if (keyval == GDK_KEY_Escape) {
        if (m_is_drawing_line) {
            m_is_drawing_line = false;
            m_first_click_for_line = false;
            clear_selection_and_highlight();
            queue_draw();
            signal_line_drawing_mode_changed.emit(false);
            std::cout << "Line drawing mode exited by ESC key." << std::endl;
            return true;
        } else if (m_is_placing_svg) { // Handle Escape for SVG placement
            m_is_placing_svg = false;
            m_svg_file_to_place = "";
            m_current_svg_placement_scale = 1.0; // Reset scale
            m_current_svg_placement_flip_horizontal = false; // Reset flip
            if (m_preview_svg_handle) { // Clean up preview handle
                g_object_unref(m_preview_svg_handle);
                m_preview_svg_handle = nullptr;
            }
            signal_svg_placement_mode_changed.emit(false);
            std::cout << "SVG placement mode exited by ESC key." << std::endl;
            queue_draw();
            return true;
        } else {
            clear_selection_and_highlight();
            queue_draw();
            std::cout << "All lines deselected by ESC key." << std::endl;
            return true;
        }
    }
    return false;
}

void DrwArea::on_key_released(guint keyval, guint , Gdk::ModifierType state)
{
    if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R) {
        m_shift_is_pressed = false;
    }
}

Glib::RefPtr<Point2D> DrwArea::find_closest_point(double mouse_x, double mouse_y, double tolerance_pixels)
{
    Glib::RefPtr<Point2D> closest_point = nullptr;
    double min_distance_sq = tolerance_pixels * tolerance_pixels;

    if (m_refTreeModel && m_Columns_ptr) {
        for (auto row : m_refTreeModel->children()) {
            for (auto child_row : row.children()) {
                ObjectType obj_type = child_row[m_Columns_ptr->m_col_object_type];
                if (obj_type == ObjectType::POINT) {
                    Glib::RefPtr<Glib::Object> generic_obj = child_row[m_Columns_ptr->m_col_generic_object];
                    
                    Glib::RefPtr<Point2D> point_obj = std::dynamic_pointer_cast<Point2D>(generic_obj);
                    if (point_obj) {
                        Glib::RefPtr<Point2D> screen_point = drawing_to_screen_coords(point_obj->x, point_obj->y);

                        double dx = screen_point->x - mouse_x;
                        double dy = screen_point->y - mouse_y;
                        double distance_sq = dx * dx + dy * dy;

                        if (distance_sq < min_distance_sq) {
                            min_distance_sq = distance_sq;
                            closest_point = point_obj;
                        }
                    }
                }
            }
        }
    }
    return closest_point;
}

Glib::RefPtr<Line> DrwArea::find_closest_line(double mouse_x, double mouse_y, double tolerance_pixels)
{
    Glib::RefPtr<Line> closest_line = nullptr;
    double min_distance_sq = tolerance_pixels * tolerance_pixels;

    if (m_refTreeModel && m_Columns_ptr) {
        for (auto row : m_refTreeModel->children()) {
            ObjectType obj_type = row[m_Columns_ptr->m_col_object_type];
            if (obj_type == ObjectType::LINE) {
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                
                Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                if (line_obj) {
                    Glib::RefPtr<Point2D> p1_screen = drawing_to_screen_coords(line_obj->start_point->x, line_obj->start_point->y);
                    Glib::RefPtr<Point2D> p2_screen = drawing_to_screen_coords(line_obj->end_point->x, line_obj->end_point->y);

                    double x1 = p1_screen->x;
                    double y1 = p1_screen->y;
                    double x2 = p2_screen->x;
                    double y2 = p2_screen->y;

                    double length_sq = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
                    if (length_sq == 0) {
                        double dist_sq = (mouse_x - x1) * (mouse_x - x1) + (mouse_y - y1) * (mouse_y - y1);
                        if (dist_sq < min_distance_sq) {
                            min_distance_sq = dist_sq;
                            closest_line = line_obj;
                        }
                        continue;
                    }

                    double t = ((mouse_x - x1) * (x2 - x1) + (mouse_y - y1) * (y2 - y1)) / length_sq;
                    t = std::max(0.0, std::min(1.0, t));

                    double closest_x = x1 + t * (x2 - x1);
                    double closest_y = y1 + t * (y2 - y1);

                    double dist_sq = (mouse_x - closest_x) * (mouse_x - closest_x) + (mouse_y - closest_y) * (mouse_y - closest_y);

                    if (dist_sq < min_distance_sq) {
                        min_distance_sq = dist_sq;
                        closest_line = line_obj;
                    }
                }
            }
        }
    }
    return closest_line;
}


void DrwArea::zoom_to_all_points()
{
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    bool found_points = false;

    if (m_refTreeModel && m_Columns_ptr) {
        for (auto row : m_refTreeModel->children()) {
            // Check for LINE objects' points
            ObjectType obj_type = row[m_Columns_ptr->m_col_object_type];
            if (obj_type == ObjectType::LINE) {
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                
                Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                if (line_obj) {
                    min_x = std::min(min_x, line_obj->start_point->x);
                    min_y = std::min(min_y, line_obj->start_point->y);
                    max_x = std::max(max_x, line_obj->start_point->x);
                    max_y = std::max(max_y, line_obj->start_point->y);
                    min_x = std::min(min_x, line_obj->end_point->x);
                    min_y = std::min(min_y, line_obj->end_point->y);
                    max_x = std::max(max_x, line_obj->end_point->x);
                    max_y = std::max(max_y, line_obj->end_point->y);
                    found_points = true;
                }
            }
            // Check for SVG objects' positions
            else if (obj_type == ObjectType::SVG) {
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                Glib::RefPtr<SvgObject> svg_obj = std::dynamic_pointer_cast<SvgObject>(generic_obj);

                if (svg_obj && svg_obj->m_svg_handle) {
                    gdouble svg_intrinsic_width, svg_intrinsic_height;
                    // Replaced deprecated rsvg_handle_get_dimensions with rsvg_handle_get_intrinsic_size_in_pixels
                    rsvg_handle_get_intrinsic_size_in_pixels(svg_obj->m_svg_handle, &svg_intrinsic_width, &svg_intrinsic_height);
                    double svg_width = svg_intrinsic_width * svg_obj->m_scale_factor;
                    double svg_height = svg_intrinsic_height * svg_obj->m_scale_factor;

                    min_x = std::min(min_x, svg_obj->m_position->x);
                    min_y = std::min(min_y, svg_obj->m_position->y);
                    max_x = std::max(max_x, svg_obj->m_position->x + svg_width);
                    max_y = std::max(max_y, svg_obj->m_position->y + svg_height);
                    found_points = true;
                }
            }

            // Also check child points if they exist (e.g., for line start/end/mid points)
            for (auto child_row : row.children()) {
                ObjectType child_obj_type = child_row[m_Columns_ptr->m_col_object_type];
                if (child_obj_type == ObjectType::POINT) {
                    Glib::RefPtr<Glib::Object> generic_obj = child_row[m_Columns_ptr->m_col_generic_object];
                   
                    Glib::RefPtr<Point2D> point_obj = std::dynamic_pointer_cast<Point2D>(generic_obj);
                    if (point_obj) {
                        min_x = std::min(min_x, point_obj->x);
                        min_y = std::min(min_y, point_obj->y);
                        max_x = std::max(max_x, point_obj->x);
                        max_y = std::max(max_y, point_obj->y);
                        found_points = true;
                    }
                }
            }
        }
    }

    if (!found_points) {
        std::cout << "No objects found to zoom to." << std::endl;
        return;
    }

    double drawing_width = max_x - min_x;
    double drawing_height = max_y - min_y;

    double margin_factor = 1.1;
    drawing_width *= margin_factor;
    drawing_height *= margin_factor;

    double area_width = get_width();
    double area_height = get_height();

    if (area_width == 0 || area_height == 0) {
        std::cerr << "Error: DrawingArea has zero dimensions." << std::endl;
        return;
    }

    double scale_x = area_width / drawing_width;
    double scale_y = area_height / drawing_height;

    m_scale = std::min(scale_x, scale_y);

    const double MIN_SCALE = 0.01;
    const double MAX_SCALE = 100.0;
    if (m_scale < MIN_SCALE) m_scale = MIN_SCALE;
    if (m_scale > MAX_SCALE) m_scale = MAX_SCALE;

    m_center_x_drawing = min_x + (max_x - min_x) / 2.0;
    m_center_y_drawing = min_y + (max_y - min_y) / 2.0;

    std::cout << "Zoom to all objects: scale=" << m_scale << ", Center: " << m_center_x_drawing << ", " << m_center_y_drawing << std::endl;
    queue_draw();
}

void DrwArea::zoom_to_a4()
{
    double a4_width = 210.0;
    double a4_height = 297.0;
    double margin_factor = 1.05;

    double target_width = a4_width * margin_factor;
    double target_height = a4_height * margin_factor;

    double area_width = static_cast<double>(get_width());
    double area_height = static_cast<double>(get_height());

    if (area_width == 0 || area_height == 0) {
        std::cerr << "Error: DrawingArea has zero dimensions for A4 zoom." << std::endl;
        return;
    }

    double scale_x = area_width / target_width;
    double scale_y = area_height / target_height;

    m_scale = std::min(scale_x, scale_y);

    const double MIN_SCALE = 0.01;
    const double MAX_SCALE = 100.0;
    if (m_scale < MIN_SCALE) m_scale = MIN_SCALE;
    if (m_scale > MAX_SCALE) m_scale = MAX_SCALE;

    m_center_x_drawing = a4_width / 2.0;
    m_center_y_drawing = a4_height / 2.0;

    std::cout << "Zoom to A4: scale=" << m_scale << ", Center: " << m_center_x_drawing << ", " << m_center_y_drawing << std::endl;
    queue_draw();
}


void DrwArea::on_mmotion(double x, double y)
{
  m_mouse_x = x;
  m_mouse_y = y;

  bool needs_redraw = false;

  if (m_pan_on){
    double delta_screen_x = x - m_last_mouse_x;
    double delta_screen_y = y - m_last_mouse_y;

    m_center_x_drawing -= delta_screen_x / m_scale;
    m_center_y_drawing += delta_screen_y / m_scale;
    needs_redraw = true;
  }
  else if (m_is_drawing_line && m_first_click_for_line)
  {
    m_line_current_mouse_pos = screen_to_drawing_coords(x, y);
    needs_redraw = true;
  }
  else if (m_is_placing_svg) // New: If in SVG placement mode, redraw to show moving SVG
  {
      needs_redraw = true;
  }

  if (m_is_drawing_line) {
      Glib::RefPtr<Point2D> current_closest_point = find_closest_point(x, y, 4.0);

      if (current_closest_point != m_highlighted_point) {
          m_highlighted_point = current_closest_point;
          needs_redraw = true;
      }
      if (m_highlighted_line) {
          m_highlighted_line = nullptr;
          needs_redraw = true;
      }
  } else {
      if (m_highlighted_point) {
          m_highlighted_point = nullptr;
          needs_redraw = true;
      }

      Glib::RefPtr<Line> current_closest_line = find_closest_line(x, y, 5.0);

      if (current_closest_line != m_highlighted_line) {
          m_highlighted_line = current_closest_line;
          needs_redraw = true;
      }
  }

  Glib::RefPtr<Point2D> current_drawing_coords = screen_to_drawing_coords(x, y);
  signal_cursor_moved.emit(current_drawing_coords->x, current_drawing_coords->y);


  if (needs_redraw)
  {
    queue_draw();
  }
  m_last_mouse_x = x;
  m_last_mouse_y = y;
}

void DrwArea::on_midb_pre(int n_press, double x, double y)
{
  m_pan_on = true;
  m_last_mouse_x = x;
  m_last_mouse_y = y;
  std::cout << "Mouse pressed on second button: "  << ", " << n_press << ", " << x << ", " << y << std::endl;
}

void DrwArea::on_midb_rel(int n_press, double x, double y)
{
  m_pan_on = false;
  std::cout << "Mouse released: " << n_press << ", " << x << ", " << y << std::endl;
}

void DrwArea::on_middle_button_double_click(int n_press, double , double )
{
    if (n_press == 2) {
        std::cout << "Middle button double click detected. Zooming to all points." << std::endl;
        zoom_to_all_points();
    }
}

void DrwArea::draw_content_to_cairo_context(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->paint();

    cr->set_line_width(1.0);
    cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

    if (m_refTreeModel && m_Columns_ptr) {
        for (auto row : m_refTreeModel->children()) {
            ObjectType obj_type = row[m_Columns_ptr->m_col_object_type];
            if (obj_type == ObjectType::LINE) {
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                
                Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                if (line_obj) {
                    cr->set_source_rgb(line_obj->color_r, line_obj->color_g, line_obj->color_b);
                    cr->set_line_width(line_obj->thickness);

                    if (line_obj->style_pattern.size() > 0) {
                        cr->set_dash(line_obj->style_pattern, 0.0);
                    } else {
                        cr->unset_dash();
                    }

                    cr->move_to(line_obj->start_point->x, line_obj->start_point->y);
                    cr->line_to(line_obj->end_point->x, line_obj->end_point->y);
                    cr->stroke();

                    cr->unset_dash();
                }
            } else if (obj_type == ObjectType::SVG) { // Draw SVG objects
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                
                Glib::RefPtr<SvgObject> svg_obj = std::dynamic_pointer_cast<SvgObject>(generic_obj);
                if (svg_obj && svg_obj->m_svg_handle) {
                    cr->save();
                    cr->translate(svg_obj->m_position->x, svg_obj->m_position->y);
                    cr->scale(svg_obj->m_scale_factor, svg_obj->m_scale_factor); // Apply SVG object's own scale factor

                    if (svg_obj->m_flip_horizontal) { // Apply horizontal flip if requested
                        // To flip horizontally while keeping the visual top-left corner at m_position,
                        // we need to translate by the width of the SVG *after* scaling, then flip.
                        // This makes the right edge of the flipped SVG align with the original left edge.
                        gdouble svg_intrinsic_width, svg_intrinsic_height;
                        rsvg_handle_get_intrinsic_size_in_pixels(svg_obj->m_svg_handle, &svg_intrinsic_width, &svg_intrinsic_height);
                        // The scale factor is already applied by cr->scale(svg_obj->m_scale_factor, svg_obj->m_scale_factor);
                        // So, svg_intrinsic_width is already the scaled width in Cairo's coordinate system.
                        cr->translate(svg_intrinsic_width, 0); // Move origin to the right edge of the scaled SVG
                        cr->scale(-1.0, 1.0); // Flip horizontally around this new origin
                    }

                    // The main view scale (m_scale) is applied at a higher level in on_draw
                    cr->scale(1.0, -1.0); // This is the vertical flip for Cairo's Y-axis

                    RsvgRectangle viewport;
                    viewport.x = 0;
                    viewport.y = 0;
                    gdouble svg_intrinsic_width, svg_intrinsic_height; // Declare here for scope
                    rsvg_handle_get_intrinsic_size_in_pixels(svg_obj->m_svg_handle, &svg_intrinsic_width, &svg_intrinsic_height);
                    viewport.width = svg_intrinsic_width; // Use intrinsic width here, as scaling is done by Cairo
                    viewport.height = svg_intrinsic_height; // Use intrinsic height here

                    rsvg_handle_render_document(svg_obj->m_svg_handle, cr->cobj(), &viewport, nullptr);
                    cr->restore();
                }
            }
            for (auto child_row : row.children()) {
                ObjectType child_obj_type = child_row[m_Columns_ptr->m_col_object_type];
                if (child_obj_type == ObjectType::POINT) {
                    Glib::RefPtr<Glib::Object> generic_obj = child_row[m_Columns_ptr->m_col_generic_object];
                    
                    Glib::RefPtr<Point2D> point_obj = std::dynamic_pointer_cast<Point2D>(generic_obj);
                    if (point_obj) {
                        cr->arc(point_obj->x, point_obj->y, 1.0, 0, 2 * G_PI);
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->fill();
                    }
                }
            }
        }
    }

    if (m_is_drawing_line && m_first_click_for_line) {
        cr->set_source_rgb(1.0, 0.0, 0.0);
        cr->set_line_width(m_current_line_thickness);
        if (m_current_line_style.size() > 0) {
            cr->set_dash(m_current_line_style, 0.0);
        } else {
            cr->unset_dash();
        }
        cr->move_to(m_line_start_point->x, m_line_start_point->y);
        cr->line_to(m_line_current_mouse_pos->x, m_line_current_mouse_pos->y);
        cr->stroke();
    }

    float a4x = 210;
    float a4y = 297;

    cr->set_line_join(Cairo::Context::LineJoin::BEVEL);
    cr->set_line_cap(Cairo::Context::LineCap::ROUND);
    cr->unset_dash();
    cr->set_source_rgb(0., 0., 0.);

    cr->set_line_width(0.18);
    cr->move_to(0, 0);
    cr->line_to(a4x, 0);
    cr->line_to(a4x, a4y);
    cr->line_to(0, a4y);
    cr->line_to(0, 0);
    cr->stroke();

    cr->set_dash(DrwArea::DASHED_PATTERN, 0.0);
    cr->set_line_width(0.18);
    cr->move_to(20, 220);
    cr->line_to(50, 220);
    cr->stroke();

    cr->set_line_width(0.25);
    cr->move_to(20, 210);
    cr->line_to(50, 210);
    cr->stroke();

    cr->set_line_width(0.35);
    cr->move_to(20, 200);
    cr->line_to(50, 200);
    cr->stroke();

    cr->set_line_width(0.7);
    cr->move_to(20, 190);
    cr->line_to(50, 190);
    cr->stroke();

    cr->set_line_width(1.);
    cr->move_to(20, 180);
    cr->line_to(50, 180);
    cr->stroke();

    cr->set_line_width(2.);
    cr->move_to(20, 170);
    cr->line_to(50, 170);
    cr->stroke();

    cr->set_line_width(3.);
    cr->move_to(20, 160);
    cr->line_to(50, 160);
    cr->stroke();

    cr->save();
    cr->scale(1.0, -1.0);
    
    auto font = Cairo::ToyFontFace::create("Sans", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_face(font);
    cr->set_font_size(20);
    cr->move_to(20, -260);
    cr->show_text("Cairomm!");

    cr->set_font_size(5);
    cr->move_to(20, -240);
    cr->show_text("Font Sans 5mm, you can also download and try ISOCPEUR font");

    cr->set_font_size(3.5);
    cr->move_to(20, -230);
    cr->show_text("Font Size 3.5mm, A4 page 210x297mm");
    cr->restore();
}


void DrwArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
    cr->save();

    cr->translate(width / 2.0, height / 2.0);
    cr->scale(m_scale, -m_scale);
    cr->translate(-m_center_x_drawing, -m_center_y_drawing);

    draw_content_to_cairo_context(cr, width, height);

    // New: Draw the SVG preview if in placement mode
    if (m_is_placing_svg && m_preview_svg_handle) {
        Glib::RefPtr<Point2D> preview_pos_drawing = screen_to_drawing_coords(m_mouse_x, m_mouse_y);

        cr->save(); // Save the current drawing transformation

        // Translate to the mouse position in drawing coordinates
        cr->translate(preview_pos_drawing->x, preview_pos_drawing->y);

        // Apply the current SVG placement scale for the preview
        cr->scale(m_current_svg_placement_scale, m_current_svg_placement_scale);

        if (m_current_svg_placement_flip_horizontal) { // Apply horizontal flip for preview
            gdouble svg_intrinsic_width, svg_intrinsic_height;
            rsvg_handle_get_intrinsic_size_in_pixels(m_preview_svg_handle, &svg_intrinsic_width, &svg_intrinsic_height);
            cr->translate(svg_intrinsic_width, 0); // Move origin to the right edge of the scaled SVG
            cr->scale(-1.0, 1.0); // Flip horizontally around this new origin
        }

        // Render the SVG. The main view scale (m_scale) is already applied above.
        cr->scale(1.0, -1.0); // This is the vertical flip for Cairo's Y-axis

        RsvgRectangle viewport;
        viewport.x = 0;
        viewport.y = 0;
        gdouble svg_intrinsic_width, svg_intrinsic_height; // Declare here for scope
        rsvg_handle_get_intrinsic_size_in_pixels(m_preview_svg_handle, &svg_intrinsic_width, &svg_intrinsic_height);
        viewport.width = svg_intrinsic_width;
        viewport.height = svg_intrinsic_height;

        rsvg_handle_render_document(m_preview_svg_handle, cr->cobj(), &viewport, nullptr);

        cr->restore(); // Restore the drawing transformation
    }

    cr->restore();

    cr->save();
    cr->translate(width / 2.0, height / 2.0);
    cr->scale(m_scale, -m_scale);
    cr->translate(-m_center_x_drawing, -m_center_y_drawing);

    if (m_refTreeModel && m_Columns_ptr) {
        for (auto row : m_refTreeModel->children()) {
            ObjectType obj_type = row[m_Columns_ptr->m_col_object_type];
            if (obj_type == ObjectType::LINE) {
                Glib::RefPtr<Glib::Object> generic_obj = row[m_Columns_ptr->m_col_generic_object];
                
                Glib::RefPtr<Line> line_obj = std::dynamic_pointer_cast<Line>(generic_obj);
                if (line_obj) {
                    bool is_currently_selected = false;
                    for (const auto& selected_line : m_selected_lines) {
                        if (selected_line == line_obj) {
                            is_currently_selected = true;
                            break;
                        }
                    }

                    if (is_currently_selected) {
                        cr->set_source_rgb(1.0, 0.65, 0.0);
                        cr->set_line_width((line_obj->thickness + 0.5) / m_scale);
                        cr->move_to(line_obj->start_point->x, line_obj->start_point->y);
                        cr->line_to(line_obj->end_point->x, line_obj->end_point->y);
                        cr->stroke();
                    } else if (m_highlighted_line && m_highlighted_line == line_obj) {
                        cr->set_source_rgb(1.0, 0.65, 0.0);
                        cr->set_line_width((line_obj->thickness + 0.25) / m_scale);
                        cr->move_to(line_obj->start_point->x, line_obj->start_point->y);
                        cr->line_to(line_obj->end_point->x, line_obj->end_point->y);
                        cr->stroke();
                    }
                }
            }
            for (auto child_row : row.children()) {
                ObjectType child_obj_type = child_row[m_Columns_ptr->m_col_object_type];
                if (child_obj_type == ObjectType::POINT) {
                    Glib::RefPtr<Glib::Object> generic_obj = child_row[m_Columns_ptr->m_col_generic_object];
                    
                    Glib::RefPtr<Point2D> point_obj = std::dynamic_pointer_cast<Point2D>(generic_obj);
                    if (point_obj) {
                        if (m_highlighted_point && m_highlighted_point == point_obj) {
                            cr->set_source_rgb(1.0, 0.0, 0.0);
                            cr->set_line_width(1.0 / m_scale);

                            double arm_length = 5.0 / m_scale;

                            cr->move_to(point_obj->x - arm_length, point_obj->y - arm_length);
                            cr->line_to(point_obj->x + arm_length, point_obj->y + arm_length);
                            cr->stroke();

                            cr->move_to(point_obj->x + arm_length, point_obj->y - arm_length);
                            cr->line_to(point_obj->x - arm_length, point_obj->y + arm_length);
                            cr->stroke();
                        }
                    }
                }
            }
        }
    }
    cr->restore();
}

void DrwArea::add_svg_object_at_coords(const Glib::ustring& filename, double click_x, double click_y)
{
    GError* error = nullptr;
    RsvgHandle* svg_handle = rsvg_handle_new_from_file(filename.c_str(), &error);
    if (!svg_handle) {
        g_printerr("Error loading SVG file '%s': %s\n", filename.c_str(), error->message);
        g_error_free(error);
        return;
    }

    Glib::RefPtr<Point2D> insert_pos = screen_to_drawing_coords(click_x, click_y);

    Glib::RefPtr<SvgObject> new_svg_obj = SvgObject::create(svg_handle, insert_pos, m_current_svg_placement_scale, m_current_svg_placement_flip_horizontal); // Use the stored scale factor and flip flag

    if (m_refTreeModel && m_Columns_ptr && m_tree_view_ptr) {
        Gtk::TreeModel::iterator parent_svg_it = m_refTreeModel->append();
        auto row = *parent_svg_it;

        row[m_Columns_ptr->m_col_id] = m_next_object_id++;
        row[m_Columns_ptr->m_col_name] = "Svg"; // Name in the tree view
        row[m_Columns_ptr->m_col_object_type] = ObjectType::SVG;
        row[m_Columns_ptr->m_col_generic_object] = new_svg_obj;
        row[m_Columns_ptr->m_col_thickness] = 0.0; // Not applicable for SVG
        row[m_Columns_ptr->m_col_style_name] = ""; // Not applicable for SVG
        row[m_Columns_ptr->m_col_color_name] = ""; // Not applicable for SVG

        std::cout << "SVG added: ID=" << row[m_Columns_ptr->m_col_id] << ", Name=" << row[m_Columns_ptr->m_col_name]
                  << ", Type=" << objectTypeToString(row[m_Columns_ptr->m_col_object_type])
                  << ", Position=(" << insert_pos->x << ", " << insert_pos->y << ")"
                  << ", Scale=" << new_svg_obj->m_scale_factor << ", Flipped Horizontally=" << new_svg_obj->m_flip_horizontal << std::endl;

        m_tree_view_ptr->expand_row(m_refTreeModel->get_path(parent_svg_it), false);
    }

    queue_draw(); // Redraw to show the new SVG
}

void DrwArea::start_svg_placement(const Glib::ustring& filename) // Modified signature
{
    m_is_placing_svg = true;
    m_svg_file_to_place = filename;
    // m_current_svg_placement_scale and m_current_svg_placement_flip_horizontal are now set externally

    // Load SVG for preview
    if (m_preview_svg_handle) { // Unref existing if any
        g_object_unref(m_preview_svg_handle);
        m_preview_svg_handle = nullptr;
    }
    GError* error = nullptr;
    m_preview_svg_handle = rsvg_handle_new_from_file(filename.c_str(), &error);
    if (!m_preview_svg_handle) {
        g_printerr("Error loading SVG for preview '%s': %s\n", filename.c_str(), error->message);
        g_error_free(error);
        // Fallback: disable placement mode if preview fails
        m_is_placing_svg = false;
        m_svg_file_to_place = "";
        m_current_svg_placement_scale = 1.0; // Reset scale
        m_current_svg_placement_flip_horizontal = false; // Reset flip
        signal_svg_placement_mode_changed.emit(false);
        return;
    }
    m_preview_svg_filename = filename;

    // Cancel line drawing mode if active
    if (m_is_drawing_line) {
        toggle_line_drawing_mode(); // This will also emit signal_line_drawing_mode_changed(false)
    }
    signal_svg_placement_mode_changed.emit(true);
    std::cout << "SVG placement mode activated. Click on drawing area to place SVG." << std::endl;
    queue_draw(); // Initial redraw to show the preview
}


void DrwArea::set_scale(double scale)
{
    m_scale = scale;
    queue_draw();
}

void DrwArea::set_center_x(double x)
{
    m_center_x_drawing = x;
    queue_draw();
}

void DrwArea::set_center_y(double y)
{
    m_center_y_drawing = y;
    queue_draw();
}


class WinMain;

// New SettingsDialog class
class SettingsDialog : public Gtk::Dialog
{
public:
    SettingsDialog(Gtk::Window& parent, double initial_scale, double initial_center_x, double initial_center_y);
    ~SettingsDialog() override;

    double get_scale() const { return m_scale_spin_button.get_value(); }
    double get_center_x() const { return m_center_x_spin_button.get_value(); }
    double get_center_y() const { return m_center_y_spin_button.get_value(); }

private:
    Gtk::Grid m_grid;
    Gtk::Label m_scale_label;
    Gtk::SpinButton m_scale_spin_button;
    Gtk::Label m_center_x_label;
    Gtk::SpinButton m_center_x_spin_button;
    Gtk::Label m_center_y_label;
    Gtk::SpinButton m_center_y_spin_button;
};

SettingsDialog::SettingsDialog(Gtk::Window& parent, double initial_scale, double initial_center_x, double initial_center_y)
    : Gtk::Dialog("Display Settings", parent, true), // Modal dialog
      m_scale_label("Scale:"),
      m_scale_spin_button(Gtk::Adjustment::create(initial_scale, 0.01, 100.0, 0.01, 0.1, 0), 2),
      m_center_x_label("Center X:"),
      m_center_x_spin_button(Gtk::Adjustment::create(initial_center_x, -1000.0, 1000.0, 1.0, 10.0, 0), 2),
      m_center_y_label("Center Y:"),
      m_center_y_spin_button(Gtk::Adjustment::create(initial_center_y, -1000.0, 1000.0, 1.0, 10.0, 0), 2)
{
    set_default_size(300, 200);
    get_content_area()->append(m_grid);
    m_grid.set_row_spacing(10);
    m_grid.set_column_spacing(10);
    m_grid.set_margin(10);

    m_grid.attach(m_scale_label, 0, 0, 1, 1);
    m_grid.attach(m_scale_spin_button, 1, 0, 1, 1);
    m_grid.attach(m_center_x_label, 0, 1, 1, 1);
    m_grid.attach(m_center_x_spin_button, 1, 1, 1, 1);
    m_grid.attach(m_center_y_label, 0, 2, 1, 1);
    m_grid.attach(m_center_y_spin_button, 1, 2, 1, 1);

    add_button("Cancel", GTK_RESPONSE_CANCEL);
    add_button("OK", GTK_RESPONSE_OK);

    set_resizable(false);
    // Removed show_children() as it's deprecated. Dialog itself will show its children.
}

SettingsDialog::~SettingsDialog()
{
}

// New LaTeXInputDialog class
class LaTeXInputDialog : public Gtk::Dialog
{
public:
    LaTeXInputDialog(Gtk::Window& parent);
    ~LaTeXInputDialog() override;

    Glib::ustring get_latex_text() const { return m_text_view.get_buffer()->get_text(); }

private:
    Gtk::Box m_box;
    Gtk::ScrolledWindow m_scrolled_window;
    Gtk::TextView m_text_view;
};

LaTeXInputDialog::LaTeXInputDialog(Gtk::Window& parent)
    : Gtk::Dialog("Enter LaTeX Expression", parent, true), // Modal dialog
      m_box(Gtk::Orientation::VERTICAL, 5)
{
    set_default_size(400, 300);
    get_content_area()->append(m_box);

    m_text_view.set_hexpand(true);
    m_text_view.set_vexpand(true);
    
    m_text_view.get_buffer()->set_text("\\ $\\alpha^2 + \\beta_1 = \\frac{1}{2}$"); // Default text
    m_scrolled_window.set_child(m_text_view);
    m_scrolled_window.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_scrolled_window.set_hexpand(true);
    m_scrolled_window.set_vexpand(true);

    // Fix for Gtk::Label* to Gtk::Widget& conversion error
    m_box.append(*Gtk::make_managed<Gtk::Label>("Enter your LaTeX expression:"));
    m_box.append(m_scrolled_window);

    add_button("Cancel", GTK_RESPONSE_CANCEL);
    add_button("OK", GTK_RESPONSE_OK);
}

LaTeXInputDialog::~LaTeXInputDialog()
{
}


class AppMenuBar : public Gtk::Box
{
public:
    AppMenuBar();
    ~AppMenuBar() override;

private:
    Gtk::Button m_FileMenuButton;
    Gtk::Button m_EditMenuButton;
    Gtk::Button m_ChoicesMenuButton;
    Gtk::Button m_OtherChoicesMenuButton;
    Gtk::Button m_HelpMenuButton;

    Gtk::PopoverMenu m_FilePopoverMenu;
    Gtk::PopoverMenu m_EditPopoverMenu;
    Gtk::PopoverMenu m_ChoicesPopoverMenu;
    Gtk::PopoverMenu m_OtherChoicesPopoverMenu;
    Gtk::PopoverMenu m_HelpPopoverMenu;
};

AppMenuBar::AppMenuBar()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0),
      m_FilePopoverMenu(Gio::Menu::create()),
      m_EditPopoverMenu(Gio::Menu::create()),
      m_ChoicesPopoverMenu(Gio::Menu::create()),
      m_OtherChoicesPopoverMenu(Gio::Menu::create()),
      m_HelpPopoverMenu(Gio::Menu::create())
{
    set_spacing(0);
    add_css_class("menu-bar");

    auto file_menu_model = Gio::Menu::create();
    file_menu_model->append("New Standard", "app.newstandard");
    file_menu_model->append("New Foo", "app.newfoo");
    file_menu_model->append("New Goo", "app.newgoo");
    file_menu_model->append_section("", Gio::Menu::create());
    file_menu_model->append("New Line", "app.newline");
    file_menu_model->append("Quit", "app.quit");

    m_FilePopoverMenu.set_menu_model(file_menu_model);
    m_FilePopoverMenu.set_has_arrow(false);
    m_FileMenuButton.set_label("File");
    m_FileMenuButton.set_has_frame(false);
    m_FileMenuButton.set_css_classes({"flat"});
    m_FileMenuButton.set_use_underline(true);
    m_FileMenuButton.signal_clicked().connect([this]() {
        m_FilePopoverMenu.set_parent(m_FileMenuButton);
        m_FilePopoverMenu.popup();
    });
    append(m_FileMenuButton);

    auto edit_menu_model = Gio::Menu::create();
    edit_menu_model->append("Copy", "win.copy");
    edit_menu_model->append("Paste", "win.paste");
    edit_menu_model->append("Something", "win.something");
    edit_menu_model->append("Delete", "win.delete");

    m_EditPopoverMenu.set_menu_model(edit_menu_model);
    m_EditPopoverMenu.set_has_arrow(false);
    m_EditMenuButton.set_label("Edit");
    m_EditMenuButton.set_has_frame(false);
    m_EditMenuButton.set_css_classes({"flat"});
    m_EditMenuButton.set_use_underline(true);
    m_EditMenuButton.signal_clicked().connect([this]() {
        m_EditPopoverMenu.set_parent(m_EditMenuButton);
        m_EditPopoverMenu.popup();
    });
    append(m_EditMenuButton);

    auto choices_menu_model = Gio::Menu::create();
    choices_menu_model->append("Choice A", "win.choice::a");
    choices_menu_model->append("Choice B", "win.choice::b");

    m_ChoicesPopoverMenu.set_menu_model(choices_menu_model);
    m_ChoicesPopoverMenu.set_has_arrow(false);
    m_ChoicesMenuButton.set_label("Choices");
    m_ChoicesMenuButton.set_has_frame(false);
    m_ChoicesMenuButton.set_css_classes({"flat"});
    m_ChoicesMenuButton.set_use_underline(true);
    m_ChoicesMenuButton.signal_clicked().connect([this]() {
        m_ChoicesPopoverMenu.set_parent(m_ChoicesMenuButton);
        m_ChoicesPopoverMenu.popup();
    });
    append(m_ChoicesMenuButton);

    auto other_choices_menu_model = Gio::Menu::create();
    other_choices_menu_model->append("Choice 1", "win.choiceother::1");
    other_choices_menu_model->append("Choice 2", "win.choiceother::2");
    other_choices_menu_model->append_section("", Gio::Menu::create());
    other_choices_menu_model->append("Some Toggle", "win.sometoggle");

    m_OtherChoicesPopoverMenu.set_menu_model(other_choices_menu_model);
    m_OtherChoicesPopoverMenu.set_has_arrow(false);
    m_OtherChoicesMenuButton.set_label("Other Choices");
    m_OtherChoicesMenuButton.set_has_frame(false);
    m_OtherChoicesMenuButton.set_css_classes({"flat"});
    m_OtherChoicesMenuButton.set_use_underline(true);
    m_OtherChoicesMenuButton.signal_clicked().connect([this]() {
        m_OtherChoicesPopoverMenu.set_parent(m_OtherChoicesMenuButton);
        m_OtherChoicesPopoverMenu.popup();
    });
    append(m_OtherChoicesMenuButton);

    auto help_menu_model = Gio::Menu::create();
    help_menu_model->append("About Window", "win.about");
    help_menu_model->append("About App", "app.about");

    m_HelpPopoverMenu.set_menu_model(help_menu_model);
    m_HelpPopoverMenu.set_has_arrow(false);
    m_HelpMenuButton.set_label("Help");
    m_HelpMenuButton.set_has_frame(false);
    m_HelpMenuButton.set_css_classes({"flat"});
    m_HelpMenuButton.set_use_underline(true);
    m_HelpMenuButton.signal_clicked().connect([this]() {
        m_HelpPopoverMenu.set_parent(m_HelpMenuButton);
        m_HelpPopoverMenu.popup();
    });
    append(m_HelpMenuButton);
}

AppMenuBar::~AppMenuBar()
{
}

class AppToolbar : public Gtk::Box
{
public:
    AppToolbar();
    ~AppToolbar() override;

    sigc::signal<void()> signal_new_line_clicked;
    sigc::signal<void()> signal_delete_clicked;
    sigc::signal<void()> signal_import_svg_clicked; // New signal for SVG import
    sigc::signal<void()> signal_import_latex_clicked; // New signal for LaTeX import
    sigc::signal<void(double)> signal_thickness_changed;
    sigc::signal<void(const std::valarray<double>&)> signal_style_changed;
    sigc::signal<void(double, double, double)> signal_color_changed;
    sigc::signal<void(guint)> signal_layer_selected;

    void set_new_line_button_active_style(bool active);
    void update_dropdowns(double thickness, const std::valarray<double>& style_pattern, double r, double g, double b, guint layer_index);
    void set_import_svg_button_active_style(bool active); // New: for SVG placement mode

private:
    Gtk::Button m_TBNewLine;
    Gtk::Button m_TBSvgImport; // New button for SVG import
    Gtk::Button m_TBLatexImport; // New button for LaTeX import
    Gtk::Button m_TBDelete;
    Gtk::DropDown m_LineThicknessDropDown;
    Gtk::DropDown m_LineStyleDropDown;
    Gtk::DropDown m_LineColorDropDown;
    Gtk::DropDown m_LineLayerDropDown;
    Gtk::Button* m_new_line_TB_ptr = nullptr;
    Gtk::Button* m_svg_import_TB_ptr = nullptr; // New: Pointer to SVG import button
    Gtk::Button* m_latex_import_TB_ptr = nullptr; // New: Pointer to LaTeX import button

    void on_TB_newline_clicked();
    void on_TB_svg_import_clicked(); // New handler for SVG import button
    void on_TB_latex_import_clicked(); // New handler for LaTeX import button
    void on_TB_delete_clicked();
    void on_line_thickness_selected();
    void on_line_style_selected();
    void on_line_color_selected();
    void on_line_layer_selected();
};

AppToolbar::AppToolbar()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0)
{
    add_css_class("toolbar");

    Gtk::Image svg_image_obj;
    svg_image_obj.set_from_resource("/toolbar/action-line-symbolic.svg");
    svg_image_obj.set_icon_size(Gtk::IconSize::NORMAL);
    m_TBNewLine.set_child(svg_image_obj);
    m_TBNewLine.set_tooltip_text("New line");
    m_TBNewLine.set_has_frame(false);
    append(m_TBNewLine);

    m_new_line_TB_ptr = &m_TBNewLine;

    // New SVG Import Button
    Gtk::Image svg_import_image_obj;
    // Replaced set_icon_name with set_from_icon_name as set_icon_name is removed in GTK4
    svg_import_image_obj.set_from_icon_name("image-x-generic-symbolic"); // Placeholder icon
    svg_import_image_obj.set_icon_size(Gtk::IconSize::NORMAL);
    m_TBSvgImport.set_child(svg_import_image_obj);
    m_TBSvgImport.set_tooltip_text("Import SVG");
    m_TBSvgImport.set_has_frame(false);
    append(m_TBSvgImport);
    m_svg_import_TB_ptr = &m_TBSvgImport; // Initialize new pointer

    // New LaTeX Import Button
    Gtk::Image latex_import_image_obj;
    latex_import_image_obj.set_from_icon_name("accessories-calculator-symbolic"); // A suitable icon for math/text
    latex_import_image_obj.set_icon_size(Gtk::IconSize::NORMAL);
    m_TBLatexImport.set_child(latex_import_image_obj);
    m_TBLatexImport.set_tooltip_text("Import LaTeX");
    m_TBLatexImport.set_has_frame(false);
    append(m_TBLatexImport);
    m_latex_import_TB_ptr = &m_TBLatexImport; // Initialize new pointer


    m_TBDelete.set_icon_name("edit-delete");
    m_TBDelete.set_tooltip_text("Delete selected");
    m_TBDelete.set_has_frame(false);
    append(m_TBDelete);

    auto line_layer_model = Gtk::StringList::create();
    line_layer_model->append("Layer 1: Red Dashed 1mm");
    line_layer_model->append("Layer 2: Blue Solid 2mm");
    line_layer_model->append("Layer 3: Green Dash-dot 0.5mm");
    m_LineLayerDropDown.set_model(line_layer_model);
    m_LineLayerDropDown.set_selected(0);
    m_LineLayerDropDown.set_tooltip_text("Select line layer preset");
    m_LineLayerDropDown.property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &AppToolbar::on_line_layer_selected)
    );
    append(m_LineLayerDropDown);

    auto line_thickness_model = Gtk::StringList::create();
    line_thickness_model->append("0.18 mm");
    line_thickness_model->append("0.25 mm");
    line_thickness_model->append("0.35 mm");
    line_thickness_model->append("0.7 mm");
    line_thickness_model->append("1.0 mm");
    line_thickness_model->append("2.0 mm");
    line_thickness_model->append("3.0 mm");
    m_LineThicknessDropDown.set_model(line_thickness_model);
    m_LineThicknessDropDown.set_selected(1);
    m_LineThicknessDropDown.set_tooltip_text("Select line thickness");
    m_LineThicknessDropDown.property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &AppToolbar::on_line_thickness_selected)
    );
    append(m_LineThicknessDropDown);

    auto line_style_model = Gtk::StringList::create();
    line_style_model->append("Solid line");
    line_style_model->append("Dashed");
    line_style_model->append("Dash-dot");
    line_style_model->append("Dash-double-dot");
    m_LineStyleDropDown.set_model(line_style_model);
    m_LineStyleDropDown.set_selected(0);
    m_LineStyleDropDown.set_tooltip_text("Select line style");
    m_LineStyleDropDown.property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &AppToolbar::on_line_style_selected)
    );
    append(m_LineStyleDropDown);

    auto line_color_model = Gtk::StringList::create();
    line_color_model->append("Black");
    line_color_model->append("Red");
    line_color_model->append("Blue");
    line_color_model->append("Green");
    m_LineColorDropDown.set_model(line_color_model);
    m_LineColorDropDown.set_selected(0);
    m_LineColorDropDown.set_tooltip_text("Select line color");
    m_LineColorDropDown.property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &AppToolbar::on_line_color_selected)
    );
    append(m_LineColorDropDown);

    m_TBNewLine.signal_clicked().connect(sigc::mem_fun(*this, &AppToolbar::on_TB_newline_clicked));
    m_TBSvgImport.signal_clicked().connect(sigc::mem_fun(*this, &AppToolbar::on_TB_svg_import_clicked)); // Connect SVG button
    m_TBLatexImport.signal_clicked().connect(sigc::mem_fun(*this, &AppToolbar::on_TB_latex_import_clicked)); // Connect LaTeX button
    m_TBDelete.signal_clicked().connect(sigc::mem_fun(*this, &AppToolbar::on_TB_delete_clicked));
}

AppToolbar::~AppToolbar()
{
}

void AppToolbar::on_TB_newline_clicked()
{
    std::cout << "AppToolbar: New line button clicked. Emitting signal." << std::endl;
    signal_new_line_clicked.emit();
}

void AppToolbar::on_TB_svg_import_clicked()
{
    std::cout << "AppToolbar: Import SVG button clicked. Emitting signal." << std::endl;
    signal_import_svg_clicked.emit();
}

void AppToolbar::on_TB_latex_import_clicked()
{
    std::cout << "AppToolbar: Import LaTeX button clicked. Emitting signal." << std::endl;
    signal_import_latex_clicked.emit();
}

void AppToolbar::on_TB_delete_clicked()
{
    std::cout << "AppToolbar: Delete button clicked. Emitting signal." << std::endl;
    signal_delete_clicked.emit();
}

void AppToolbar::on_line_thickness_selected()
{
    guint selected_index = m_LineThicknessDropDown.get_selected();
    double thickness = 0.18;

    if (selected_index != GTK_INVALID_LIST_POSITION) {
        Glib::RefPtr<Gtk::StringObject> selected_object =
            std::dynamic_pointer_cast<Gtk::StringObject>(m_LineThicknessDropDown.get_selected_item());

        if (selected_object) {
            Glib::ustring selected_text = selected_object->get_string();
            try {
                size_t start_pos = selected_text.find_first_of("0123456789.");
                if (start_pos != Glib::ustring::npos) {
                    size_t end_pos = selected_text.find_first_not_of("0123456789.", start_pos);
                    if (end_pos == Glib::ustring::npos) {
                        end_pos = selected_text.length();
                    }
                    thickness = std::stod(selected_text.substr(start_pos, end_pos - start_pos));
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line thickness in AppToolbar: " << e.what() << std::endl;
                thickness = 0.18;
            }
        }
    }
    std::cout << "AppToolbar: Selected line thickness: " << thickness << " mm. Emitting signal." << std::endl;
    signal_thickness_changed.emit(thickness);
}

void AppToolbar::on_line_style_selected()
{
    guint selected_index = m_LineStyleDropDown.get_selected();
    std::valarray<double> style_pattern = {};

    if (selected_index != GTK_INVALID_LIST_POSITION) {
        if (selected_index == 0) {
            style_pattern = {};
        } else if (selected_index == 1) {
            style_pattern = DrwArea::DASHED_PATTERN;
        } else if (selected_index == 2) {
            style_pattern = DrwArea::DASHDOTED_PATTERN;
        } else if (selected_index == 3) {
            style_pattern = DrwArea::DASHDDOTTED_PATTERN;
        }
    }
    std::cout << "AppToolbar: Selected line style index: " << selected_index << ". Emitting signal." << std::endl;
    signal_style_changed.emit(style_pattern);
}

void AppToolbar::on_line_color_selected()
{
    guint selected_index = m_LineColorDropDown.get_selected();
    double r = 0.0, g = 0.0, b = 0.0;

    if (selected_index != GTK_INVALID_LIST_POSITION) {
        Glib::RefPtr<Gtk::StringObject> selected_object =
            std::dynamic_pointer_cast<Gtk::StringObject>(m_LineColorDropDown.get_selected_item());

        if (selected_object) {
            Glib::ustring selected_text = selected_object->get_string();
            if (selected_text == "Black") {
                r = 0.0; g = 0.0; b = 0.0;
            } else if (selected_text == "Red") {
                r = 1.0; g = 0.0; b = 0.0;
            } else if (selected_text == "Blue") {
                r = 0.0; g = 0.0; b = 1.0;
            } else if (selected_text == "Green") {
                r = 0.0; g = 1.0; b = 0.0;
            }
        }
    }
    std::cout << "AppToolbar: Selected line color: R" << r << " G" << g << " B" << b << ". Emitting signal." << std::endl;
    signal_color_changed.emit(r, g, b);
}

void AppToolbar::on_line_layer_selected()
{
    guint selected_index = m_LineLayerDropDown.get_selected();
    std::cout << "AppToolbar: Selected line layer index: " << selected_index << ". Emitting signal." << std::endl;
    signal_layer_selected.emit(selected_index);
}

void AppToolbar::set_new_line_button_active_style(bool active)
{
    if (m_new_line_TB_ptr) {
        if (active) {
            m_new_line_TB_ptr->get_style_context()->add_class("new-line-active");
        } else {
            m_new_line_TB_ptr->get_style_context()->remove_class("new-line-active");
        }
    }
}

void AppToolbar::set_import_svg_button_active_style(bool active)
{
    if (m_svg_import_TB_ptr) {
        if (active) {
            m_svg_import_TB_ptr->get_style_context()->add_class("new-line-active"); // Reusing same style for now
        } else {
            m_svg_import_TB_ptr->get_style_context()->remove_class("new-line-active");
        }
    }
}

void AppToolbar::update_dropdowns(double thickness, const std::valarray<double>& style_pattern, double r, double g, double b, guint layer_index)
{
    if (thickness == 0.18) m_LineThicknessDropDown.set_selected(0);
    else if (thickness == 0.25) m_LineThicknessDropDown.set_selected(1);
    else if (thickness == 0.35) m_LineThicknessDropDown.set_selected(2);
    else if (thickness == 0.7) m_LineThicknessDropDown.set_selected(3);
    else if (thickness == 1.0) m_LineThicknessDropDown.set_selected(4);
    else if (thickness == 2.0) m_LineThicknessDropDown.set_selected(5);
    else if (thickness == 3.0) m_LineThicknessDropDown.set_selected(6);
    else m_LineThicknessDropDown.set_selected(GTK_INVALID_LIST_POSITION);

    if (style_pattern.size() == 0) m_LineStyleDropDown.set_selected(0);
    else if (style_pattern.size() == 2 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3) m_LineStyleDropDown.set_selected(1);
    else if (style_pattern.size() == 4 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3 && style_pattern[2] == .3 && style_pattern[3] == 1.3) m_LineStyleDropDown.set_selected(2);
    else if (style_pattern.size() == 6 && style_pattern[0] == 3.3 && style_pattern[1] == 1.3 && style_pattern[2] == .3 && style_pattern[3] == 1.3 && style_pattern[4] == .3 && style_pattern[5] == 1.3) m_LineStyleDropDown.set_selected(3);
    else m_LineStyleDropDown.set_selected(GTK_INVALID_LIST_POSITION);

    if (r == 0.0 && g == 0.0 && b == 0.0) m_LineColorDropDown.set_selected(0);
    else if (r == 1.0 && g == 0.0 && b == 0.0) m_LineColorDropDown.set_selected(1);
    else if (r == 0.0 && g == 0.0 && b == 1.0) m_LineColorDropDown.set_selected(2);
    else if (r == 0.0 && g == 1.0 && b == 0.0) m_LineColorDropDown.set_selected(3);
    else m_LineColorDropDown.set_selected(GTK_INVALID_LIST_POSITION);

    if (layer_index != GTK_INVALID_LIST_POSITION) {
        m_LineLayerDropDown.set_selected(layer_index);
    }
}

class AppStatusBar : public Gtk::Box
{
public:
    AppStatusBar();
    ~AppStatusBar() override;

    sigc::signal<void(double, double)> signal_coordinates_entered;
    sigc::signal<void(double)> signal_scale_entered; // New signal for scale input

    void update_line_mode_status(bool active);
    void update_cursor_coords(double x, double y);
    void update_svg_placement_status(bool active); // New: for SVG placement mode
    Gtk::Entry& get_entry() { return m_status_entry; }

    void set_svg_placement_mode_active(bool active) { m_is_svg_placement_mode_active = active; } // New setter for mode

private:
    Gtk::Label m_status_message_label;
    Gtk::Entry m_status_entry;
    Glib::ustring m_base_status_message;
    bool m_is_svg_placement_mode_active; // New member to track SVG placement mode

    void on_status_entry_activate();
};

AppStatusBar::AppStatusBar()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0),
      m_is_svg_placement_mode_active(false) // Initialize to false
{
    set_margin_top(5);
    set_margin_bottom(5);
    set_margin_start(10);
    set_margin_end(10);
    set_spacing(10);
    set_css_classes({"custom-statusbar"});

    m_status_message_label.set_hexpand();
    m_status_message_label.set_halign(Gtk::Align::START);
    m_base_status_message = "Ready. Click or enter coordinates.";
    m_status_message_label.set_label(m_base_status_message);
    append(m_status_message_label);

    m_status_entry.set_hexpand(false);
    m_status_entry.set_halign(Gtk::Align::END);
    m_status_entry.set_placeholder_text("Enter x,y coordinates...");
    m_status_entry.signal_activate().connect(
        sigc::mem_fun(*this, &AppStatusBar::on_status_entry_activate));
    append(m_status_entry);
}

AppStatusBar::~AppStatusBar()
{
}

void AppStatusBar::update_line_mode_status(bool active)
{
    if (active) {
        m_base_status_message = "New line mode started. Click or enter coordinates.";
    } else {
        m_base_status_message = "New line mode ended.";
    }
    m_status_message_label.set_label(m_base_status_message);
}

void AppStatusBar::update_svg_placement_status(bool active)
{
    if (active) {
        m_base_status_message = "SVG placement mode active. Click on drawing area to place.";
    } else {
        m_base_status_message = "SVG placement mode ended.";
    }
    m_status_message_label.set_label(m_base_status_message);
}


void AppStatusBar::update_cursor_coords(double x, double y)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << x << ", " << y;
    m_status_message_label.set_label(m_base_status_message + " Cursor: (" + ss.str() + ")");
}

void AppStatusBar::on_status_entry_activate()
{
    Glib::ustring text = m_status_entry.get_text();
    if (text.empty()) return;

    if (m_is_svg_placement_mode_active) {
        try {
            double scale = std::stod(text.raw());
            if (scale > 0) { // Ensure scale is positive
                signal_scale_entered.emit(scale);
                m_base_status_message = "SVG scale set. Click on drawing area to place.";
                m_status_message_label.set_label(m_base_status_message);
                std::cout << "SVG scale entered: " << scale << std::endl;
            } else {
                m_base_status_message = "Input error: Scale must be positive.";
                m_status_message_label.set_label(m_base_status_message);
                std::cerr << "Input error: Scale must be positive." << std::endl;
            }
        } catch (const std::exception& e) {
            m_base_status_message = "Input error: Invalid scale number.";
            m_status_message_label.set_label(m_base_status_message);
            std::cerr << "Input error: " << e.what() << std::endl;
        }
    } else {
        // Existing x,y parsing logic
        size_t comma_pos = text.find(',');
        if (comma_pos != Glib::ustring::npos && comma_pos > 0 && comma_pos < text.length() - 1)
        {
            try {
                double parsed_x = std::stod(text.substr(0, comma_pos));
                double parsed_y = std::stod(text.substr(comma_pos + 1));
                signal_coordinates_entered.emit(parsed_x, parsed_y);
            } catch (const std::invalid_argument& e) {
                m_base_status_message = "Input error: Invalid numbers. Format: x,y";
                m_status_message_label.set_label(m_base_status_message);
                std::cerr << "Input error: " << e.what() << std::endl;
            } catch (const std::out_of_range& e) {
                m_base_status_message = "Input error: Numbers out of range. Format: x,y";
                m_status_message_label.set_label(m_base_status_message);
                std::cerr << "Input error: " << e.what() << std::endl;
            }
        } else {
            m_base_status_message = "Invalid format. Use 'x,y'.";
            m_status_message_label.set_label(m_base_status_message);
            std::cout << "Invalid input format: " << text << std::endl;
        }
    }
    m_status_entry.set_text(""); // Clear entry after processing
}


class WinMain : public Gtk::ApplicationWindow
{
public:
  WinMain();
  ~WinMain() override;

protected:
  void on_title_entry_changed();
  void on_button_clicked();
  void on_menu_others();

  void on_menu_choices(const Glib::VariantBase& parameter);
  void on_menu_choices_other(const Glib::VariantBase& parameter);

  void on_menu_toggle();

  void on_statusbar_coordinates_entered(double x, double y);
  void on_statusbar_scale_entered(double scale); // New handler for scale input

  void on_menu_file_delete();

  void on_toolbar_newline_clicked();
  void on_toolbar_delete_clicked();
  void on_toolbar_import_svg_clicked(); // New handler for SVG import
  void on_toolbar_import_latex_clicked(); // New handler for LaTeX import
  void on_toolbar_thickness_changed(double thickness);
  void on_toolbar_style_changed(const std::valarray<double>& style);
  void on_toolbar_color_changed(double r, double g, double b);
  void on_toolbar_layer_selected(guint layer_index);

  void on_TB_export_pdf_clicked();
  void export_lines_to_pdf(const Glib::ustring& filename);

  void on_TB_print_clicked();
  void on_print_begin(const Glib::RefPtr<Gtk::PrintContext>& context);
  void on_print_draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr);
  void on_TB_settings_clicked(); // New handler for settings button


  Gtk::Box m_Box;
  Gtk::Box m_DrawingAreaContainer; // New container for DrwArea and StackSwitcher

  Gtk::Paned m_VPaned;
  TreeList m_TreeList;
  DrwArea m_DrwArea;
  Gtk::Stack m_Stack; // Declared Gtk::Stack
  Gtk::StackSwitcher m_StackSwitcher; // Declared Gtk::StackSwitcher

  AppStatusBar m_AppStatusBar;

  Glib::RefPtr<Gio::SimpleAction> m_refChoice;
  Glib::RefPtr<Gio::SimpleAction> m_refChoiceOther;

  Glib::RefPtr<Gio::SimpleAction> m_refToggle;

  Gtk::HeaderBar m_header_bar;
  Gtk::Label m_title_buttons_label;

  Glib::RefPtr<Gtk::CssProvider> m_css_provider;

    void on_TB_new_clicked();
    void on_TB_open_clicked();
    void on_TB_save_clicked();

    Gtk::Button m_HeaderBarNew;
    Gtk::Button m_HeaderBarOpen;
    Gtk::Button m_HeaderBarSave;
    Gtk::Button m_HeaderBarExportPdf;
    Gtk::Button m_HeaderBarPrint;
    Gtk::Button m_HeaderBarSettings; // New settings button

    Glib::RefPtr<Gtk::PrintOperation> m_current_print_operation;


    AppMenuBar m_AppMenuBar;
    AppToolbar m_AppToolbar;

public:
  DrwArea& get_my_area() { return m_DrwArea; }
  TreeList& get_tree_list() { return m_TreeList; }
  Gtk::Entry& get_status_entry() { return m_AppStatusBar.get_entry(); }
};


WinMain::WinMain()
  : Gtk::ApplicationWindow(),
    m_Box(Gtk::Orientation::VERTICAL),
    m_DrawingAreaContainer(Gtk::Orientation::VERTICAL, 0), // Initialize new container
    m_title_buttons_label("Show title buttons:", Gtk::Align::END, Gtk::Align::CENTER)
{
  set_title("Cad2d App");
  set_default_size(1024, 768);

  set_child(m_Box);

  m_header_bar.set_show_title_buttons(true);

  m_HeaderBarNew.set_icon_name("document-new");
  m_HeaderBarNew.set_tooltip_text("New file");
  m_HeaderBarNew.set_has_frame(false);
  m_HeaderBarNew.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_new_clicked));
  m_header_bar.pack_start(m_HeaderBarNew);

  m_HeaderBarOpen.set_icon_name("document-open");
  m_HeaderBarOpen.set_tooltip_text("Open file");
  m_HeaderBarOpen.set_has_frame(false);
  m_HeaderBarOpen.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_open_clicked));
  m_header_bar.pack_start(m_HeaderBarOpen);

  m_HeaderBarSave.set_icon_name("document-save");
  m_HeaderBarSave.set_tooltip_text("Save file");
  m_HeaderBarSave.set_has_frame(false);
  m_HeaderBarSave.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_save_clicked));
  m_header_bar.pack_start(m_HeaderBarSave);

  m_HeaderBarExportPdf.set_icon_name("document-export-symbolic");
  m_HeaderBarExportPdf.set_tooltip_text("Export to PDF");
  m_HeaderBarExportPdf.set_has_frame(false);
  m_HeaderBarExportPdf.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_export_pdf_clicked));
  m_header_bar.pack_start(m_HeaderBarExportPdf);

  m_HeaderBarPrint.set_icon_name("document-print-symbolic");
  m_HeaderBarPrint.set_tooltip_text("Print drawing");
  m_HeaderBarPrint.set_has_frame(false);
  m_HeaderBarPrint.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_print_clicked));
  m_header_bar.pack_start(m_HeaderBarPrint);

  // New settings button
  m_HeaderBarSettings.set_icon_name("preferences-system-symbolic");
  m_HeaderBarSettings.set_tooltip_text("Settings");
  m_HeaderBarSettings.set_has_frame(false);
  m_HeaderBarSettings.signal_clicked().connect(sigc::mem_fun(*this, &WinMain::on_TB_settings_clicked));
  m_header_bar.pack_end(m_HeaderBarSettings); // Pack to the end (right side)


  set_titlebar(m_header_bar);

  m_Box.append(m_AppMenuBar);
  m_Box.append(m_AppToolbar);

  // Ensure the drawing area container expands
  m_DrawingAreaContainer.set_vexpand(true);
  m_DrawingAreaContainer.set_hexpand(true);

  // Link StackSwitcher to Stack and add it to the new drawing area container
  m_StackSwitcher.set_stack(m_Stack);
  m_StackSwitcher.set_hexpand(false); // Prevent horizontal expansion
  m_StackSwitcher.set_halign(Gtk::Align::START); // Align to start (left)
  m_DrawingAreaContainer.append(m_StackSwitcher); // Add StackSwitcher to the new container

  m_VPaned.set_margin(10);

  m_VPaned.set_start_child(m_TreeList);
  m_VPaned.set_wide_handle(true); // Make the handle wider for easier dragging

  // Corrected: Capture the Glib::RefPtr<Gtk::StackPage> and then use it.
  Glib::RefPtr<Gtk::StackPage> drawing_page_ref = m_Stack.add(m_DrwArea);
  drawing_page_ref->set_name("drawing_area");
  drawing_page_ref->set_title("Drawing Area");

  // Ensure DrwArea itself expands within the stack page
  m_DrwArea.set_vexpand(true);
  m_DrwArea.set_hexpand(true);

  // Add another page to demonstrate StackSwitcher
  auto label_page = Gtk::make_managed<Gtk::Label>("This is the second page.");
  Glib::RefPtr<Gtk::StackPage> second_page_ref = m_Stack.add(*label_page);
  second_page_ref->set_name("second_page");
  second_page_ref->set_title("Second Page");

  m_Stack.set_visible_child(m_DrwArea); // Set DrwArea as the initially visible child
  m_Stack.set_vexpand(true); // Ensure the stack expands vertically
  m_Stack.set_hexpand(true); // Ensure the stack expands horizontally
  m_DrawingAreaContainer.append(m_Stack); // Add the stack to the new container

  m_VPaned.set_end_child(m_DrawingAreaContainer); // Set the new container as the end child of the paned


  add_action("copy", sigc::mem_fun(*this, &WinMain::on_menu_others));
  add_action("paste", sigc::mem_fun(*this, &WinMain::on_menu_others));
  add_action("something", sigc::mem_fun(*this, &WinMain::on_menu_others));
  add_action("delete", sigc::mem_fun(*this, &WinMain::on_menu_file_delete));

  m_refChoice = Gio::SimpleAction::create("choice", Glib::VariantType("s"), Glib::Variant<Glib::ustring>::create("a"));
  add_action(m_refChoice);
  m_refChoice->signal_activate().connect(sigc::mem_fun(*this, &WinMain::on_menu_choices));

  m_refChoiceOther = Gio::SimpleAction::create("choiceother", Glib::VariantType("i"), Glib::Variant<int>::create(1));
  add_action(m_refChoiceOther);
  m_refChoiceOther->signal_activate().connect(sigc::mem_fun(*this, &WinMain::on_menu_choices_other));

  m_refToggle = add_action_bool("sometoggle",
    sigc::mem_fun(*this, &WinMain::on_menu_toggle), false);

  m_Box.append(m_VPaned);
  m_VPaned.set_vexpand(true);
  // Set a smaller position for the TreeList to give more space to the drawing area
  m_VPaned.set_position(150);

  m_Box.append(m_AppStatusBar);


  m_css_provider = Gtk::CssProvider::create();
  m_css_provider->load_from_data(
      ".new-line-active {\n"
      "  background-color: #ff8c00; \n"
      "  border-width: 2px;\n"
      "  border-style: solid;\n"
      "  border-color: #0000ff;\n"
      "}\n"
      ".custom-statusbar {\n"
      "  background-color: #e0e0e0; \n"
      "  border-top: 1px solid #c0c0c0; \n"
      "}\n"
      ".menu-bar { background-color: @theme_bg_color; border-bottom: 1px solid @theme_border_color; }\n"
      ".toolbar { background-color: #f0f0f0; border-bottom: 1px solid #d0d0d0; padding: 5px; }\n"
  );
  Gtk::StyleContext::add_provider_for_display(
      Gdk::Display::get_default(), m_css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );

  m_DrwArea.signal_line_drawing_mode_changed.connect(
      sigc::mem_fun(m_AppStatusBar, &AppStatusBar::update_line_mode_status));
  m_DrwArea.signal_cursor_moved.connect(
      sigc::mem_fun(m_AppStatusBar, &AppStatusBar::update_cursor_coords));
  m_DrwArea.signal_svg_placement_mode_changed.connect( // Connect new signal
      sigc::mem_fun(m_AppStatusBar, &AppStatusBar::update_svg_placement_status));


  m_AppStatusBar.signal_coordinates_entered.connect(
      sigc::mem_fun(*this, &WinMain::on_statusbar_coordinates_entered));
  m_AppStatusBar.signal_scale_entered.connect( // Connect new scale signal
      sigc::mem_fun(*this, &WinMain::on_statusbar_scale_entered));


  m_AppToolbar.signal_new_line_clicked.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_newline_clicked));
  m_AppToolbar.signal_delete_clicked.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_delete_clicked));
  m_AppToolbar.signal_import_svg_clicked.connect( // Connect new SVG import signal
      sigc::mem_fun(*this, &WinMain::on_toolbar_import_svg_clicked));
  m_AppToolbar.signal_import_latex_clicked.connect( // Connect new LaTeX import signal
      sigc::mem_fun(*this, &WinMain::on_toolbar_import_latex_clicked));
  m_AppToolbar.signal_thickness_changed.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_thickness_changed));
  m_AppToolbar.signal_style_changed.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_style_changed));
  m_AppToolbar.signal_color_changed.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_color_changed));
  m_AppToolbar.signal_layer_selected.connect(
      sigc::mem_fun(*this, &WinMain::on_toolbar_layer_selected));

  m_DrwArea.signal_line_drawing_mode_changed.connect(
      sigc::mem_fun(m_AppToolbar, &AppToolbar::set_new_line_button_active_style));
  m_DrwArea.signal_svg_placement_mode_changed.connect( // Connect new signal
      sigc::mem_fun(m_AppToolbar, &AppToolbar::set_import_svg_button_active_style));


  m_DrwArea.set_tree_store(m_TreeList.m_refTreeModel, m_TreeList.m_Columns, &m_TreeList.m_TreeView);
}

WinMain::~WinMain()
{
}

void WinMain::on_TB_new_clicked()
{
    std::cout << "New file clicked!" << std::endl;
}

void WinMain::on_TB_open_clicked()
{
    std::cout << "Open file clicked!" << std::endl;
}

void WinMain::on_TB_save_clicked()
{
    std::cout << "Save file clicked!" << std::endl;
}

void WinMain::on_TB_export_pdf_clicked()
{
    std::cout << "Export to PDF clicked!" << std::endl;

    auto dialog = Gtk::FileChooserNative::create(
        "Export to PDF",
        *this,
        Gtk::FileChooserNative::Action::SAVE,
        "Save",
        "Cancel"
    );

    auto filter_pdf = Gtk::FileFilter::create();
    filter_pdf->set_name("PDF files (*.pdf)");
    filter_pdf->add_pattern("*.pdf");
    dialog->add_filter(filter_pdf);

    dialog->set_current_name("export.pdf");

    dialog->signal_response().connect([this, dialog_ref = dialog](int response_id) {
        if (response_id == GTK_RESPONSE_ACCEPT) {
            auto file = dialog_ref->get_file();
            if (file) {
                Glib::ustring path = file->get_path();
                if (!path.empty()) {
                    export_lines_to_pdf(path);
                    std::cout << "PDF exported to: " << path << std::endl;
                }
            }
        } else {
                    std::cout << "PDF export cancelled." << std::endl;
                }
            });

    dialog->show();
}

void WinMain::export_lines_to_pdf(const Glib::ustring& filename)
{
    double width_in_points = 210.0 * (72.0 / 25.4);
    double height_in_points = 297.0 * (72.0 / 25.4);

    try {
        auto pdf_surface = Cairo::PdfSurface::create(filename, width_in_points, height_in_points);
        auto cr = Cairo::Context::create(pdf_surface);

        double mm_to_points_scale = 72.0 / 25.4;
        cr->scale(mm_to_points_scale, mm_to_points_scale);

        cr->translate(0.0, height_in_points / mm_to_points_scale);
        cr->scale(1.0, -1.0);

        m_DrwArea.draw_content_to_cairo_context(cr, 210, 297);

        cr->show_page();
        pdf_surface->finish();

        std::cout << "PDF export successful to: " << filename << std::endl;
    } catch (const Glib::Error& ex) {
        std::cerr << "Error exporting to PDF: " << ex.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Standard exception during PDF export: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during PDF export!" << std::endl;
    }
}


void WinMain::on_TB_print_clicked()
{
    std::cout << "Print button clicked!" << std::endl;
    m_current_print_operation = Gtk::PrintOperation::create();

    m_current_print_operation->signal_begin_print().connect(
        sigc::mem_fun(*this, &WinMain::on_print_begin)
    );
    m_current_print_operation->signal_draw_page().connect(
        sigc::mem_fun(*this, &WinMain::on_print_draw_page)
    );

    try {
        m_current_print_operation->run(Gtk::PrintOperation::Action::PRINT_DIALOG, *this);
    } catch (const Glib::Error& ex) {
        std::cerr << "Error running print operation: " << ex.what() << std::endl;
    }
}

void WinMain::on_print_begin(const Glib::RefPtr<Gtk::PrintContext>& context)
{
    if (m_current_print_operation) {
        m_current_print_operation->set_n_pages(1);
    }
    std::cout << "Print operation began. Number of pages: 1" << std::endl;
}

void WinMain::on_print_draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr)
{
    if (page_nr != 0) return;

    auto cr = context->get_cairo_context();
    double width_points = context->get_width();
    double height_points = context->get_height();

    double scale_factor_mm_to_points = 72.0 / 25.4;

    cr->save();

    cr->scale(scale_factor_mm_to_points, scale_factor_mm_to_points);

    cr->translate(0.0, height_points / scale_factor_mm_to_points);
    cr->scale(1.0, -1.0);

    m_DrwArea.draw_content_to_cairo_context(cr, width_points / scale_factor_mm_to_points, height_points / scale_factor_mm_to_points);

    cr->restore();
    std::cout << "Drawing page " << page_nr << " for printing." << std::endl;
}

void WinMain::on_TB_settings_clicked()
{
    std::cout << "Settings button clicked!" << std::endl;
    // Create the dialog using current values from DrwArea via getters
    auto dialog = Gtk::make_managed<SettingsDialog>(
        *this,
        m_DrwArea.get_scale(),
        m_DrwArea.get_center_x_drawing(),
        m_DrwArea.get_center_y_drawing()
    );

    // Connect to the response signal
    dialog->signal_response().connect([this, dialog_ptr = dialog](int response_id) {
        if (response_id == GTK_RESPONSE_OK) {
            m_DrwArea.set_scale(dialog_ptr->get_scale());
            m_DrwArea.set_center_x(dialog_ptr->get_center_x());
            m_DrwArea.set_center_y(dialog_ptr->get_center_y());
            std::cout << "Settings applied: Scale=" << dialog_ptr->get_scale()
                      << ", Center X=" << dialog_ptr->get_center_x()
                      << ", Center Y=" << dialog_ptr->get_center_y() << std::endl;
        } else {
            std::cout << "Settings dialog cancelled." << std::endl;
        }
        // Dialog is automatically destroyed when response is handled and it's a managed widget
    });

    dialog->show(); // Show the dialog
}


void WinMain::on_toolbar_newline_clicked()
{
  std::cout << "WinMain: New line action from toolbar. Toggling line drawing mode." << std::endl;
  m_DrwArea.toggle_line_drawing_mode();
  get_status_entry().grab_focus();
}

void WinMain::on_toolbar_import_svg_clicked()
{
    std::cout << "WinMain: Import SVG action from toolbar. Opening file chooser." << std::endl;

    auto dialog = Gtk::FileChooserNative::create(
        "Import SVG File",
        *this,
        Gtk::FileChooserNative::Action::OPEN,
        "Open",
        "Cancel"
    );

    auto filter_svg = Gtk::FileFilter::create();
    filter_svg->set_name("SVG files (*.svg)");
    filter_svg->add_pattern("*.svg");
    dialog->add_filter(filter_svg);

    dialog->signal_response().connect([this, dialog_ref = dialog](int response_id) {
        if (response_id == GTK_RESPONSE_ACCEPT) {
            auto file = dialog_ref->get_file();
            if (file) {
                Glib::ustring path = file->get_path();
                if (!path.empty()) {
                    // Set default scale and flip for non-LaTeX SVG
                    m_DrwArea.set_current_svg_placement_scale(1.0); // Default scale
                    m_DrwArea.set_current_svg_placement_flip_horizontal(false); // Default flip

                    // Start SVG placement mode without specifying scale yet
                    m_DrwArea.start_svg_placement(path);
                    // Inform status bar about SVG placement mode
                    m_AppStatusBar.set_svg_placement_mode_active(true);
                    m_AppStatusBar.get_entry().set_placeholder_text("Enter scale (e.g., 2.5) or click to place...");
                    m_AppStatusBar.update_svg_placement_status(true);
                    std::cout << "SVG file selected: " << path << ". Enter scale in status bar or click to place with default scale." << std::endl;
                }
            }
        } else {
            std::cout << "SVG import cancelled." << std::endl;
        }
    });

    dialog->show();
}

void WinMain::on_toolbar_import_latex_clicked()
{
    std::cout << "WinMain: Import LaTeX action from toolbar. Opening LaTeX input dialog." << std::endl;

    auto dialog = Gtk::make_managed<LaTeXInputDialog>(*this);

    dialog->signal_response().connect([this, dialog_ptr = dialog](int response_id) {
        if (response_id == GTK_RESPONSE_OK) {
            Glib::ustring latex_text = dialog_ptr->get_latex_text();
            if (!latex_text.empty()) {
                std::cout << "LaTeX text entered: " << latex_text << std::endl;

                // Define a temporary filename for the SVG output
                Glib::ustring temp_svg_filename = "tmp_latex.svg";
                Glib::ustring temp_base_filename = "tmp_latex";
                int result = latex2svg(latex_text.raw(), temp_base_filename.raw());

                if (result == 0) {
                    std::cout << "LaTeX converted to SVG successfully: " << temp_svg_filename << std::endl;
                    // Set fixed scale and flip for LaTeX-generated SVG
                    m_DrwArea.set_current_svg_placement_scale(25.4 / 72.0); // Fixed scale for LaTeX
                    m_DrwArea.set_current_svg_placement_flip_horizontal(false); // No horizontal flip for LaTeX

                    m_DrwArea.start_svg_placement(temp_svg_filename); // Start placement with pre-set scale/flip
                    m_AppStatusBar.set_svg_placement_mode_active(true);
                    m_AppStatusBar.get_entry().set_placeholder_text("Click on drawing area to place SVG...");
                    m_AppStatusBar.update_svg_placement_status(true);
                } else {
                    std::cerr << "Error converting LaTeX to SVG. Return code: " << result << std::endl;
                    // Fix for Gtk::MessageDialog::create error
                    auto error_dialog = Gtk::make_managed<Gtk::MessageDialog>(
                        *this, // Parent window
                        Glib::ustring("Failed to convert LaTeX to SVG."), // Primary message
                        false, // use_markup
                        Gtk::MessageType::ERROR,
                        Gtk::ButtonsType::OK
                    );
                    error_dialog->set_title(Glib::ustring("LaTeX Conversion Error")); // Set the title
                    error_dialog->set_secondary_text(Glib::ustring("Please check your LaTeX syntax and ensure pdflatex and pdf2svg are installed and in your PATH. Also check the console for pdflatex log output.")); // Added console log hint
                    error_dialog->set_modal(true); // Make it modal
                    error_dialog->show();
                }
            } else {
                std::cout << "LaTeX input was empty." << std::endl;
            }
        } else {
            std::cout << "LaTeX import cancelled." << std::endl;
        }
    });

    dialog->show();
}


void WinMain::on_toolbar_delete_clicked()
{
    on_menu_file_delete();
}

void WinMain::on_toolbar_thickness_changed(double thickness)
{
    m_DrwArea.set_line_thickness(thickness);
}

void WinMain::on_toolbar_style_changed(const std::valarray<double>& style)
{
    m_DrwArea.set_line_style(style);
}

void WinMain::on_toolbar_color_changed(double r, double g, double b)
{
    m_DrwArea.set_line_color(r, g, b);
}

void WinMain::on_toolbar_layer_selected(guint layer_index)
{
    if (layer_index != GTK_INVALID_LIST_POSITION) {
        if (layer_index == 0) {
            m_DrwArea.set_line_thickness(1.0);
            m_DrwArea.set_line_style(DrwArea::DASHED_PATTERN);
            m_DrwArea.set_line_color(1.0, 0.0, 0.0);
            std::cout << "WinMain: Selected Layer 1: Red Dashed 1mm" << std::endl;
        } else if (layer_index == 1) {
            m_DrwArea.set_line_thickness(2.0);
            m_DrwArea.set_line_style({});
            m_DrwArea.set_line_color(0.0, 0.0, 1.0);
            std::cout << "WinMain: Selected Layer 2: Blue Solid 2mm" << std::endl;
        } else if (layer_index == 2) {
            m_DrwArea.set_line_thickness(0.5);
            m_DrwArea.set_line_style(DrwArea::DASHDOTED_PATTERN);
            m_DrwArea.set_line_color(0.0, 1.0, 0.0);
            std::cout << "WinMain: Selected Layer 3: Green Dash-dot 0.5mm" << std::endl;
        }
        m_AppToolbar.update_dropdowns(
            m_DrwArea.get_current_line_thickness(),
            m_DrwArea.get_current_line_style(),
            m_DrwArea.get_current_line_color_r(),
            m_DrwArea.get_current_line_color_g(),
            m_DrwArea.get_current_line_color_b(),
            layer_index
        );
    }
}


void WinMain::on_statusbar_coordinates_entered(double x, double y)
{
    // If SVG placement mode is active, this input is for scale, not coordinates
    if (m_DrwArea.get_is_placing_svg()) {
        // This case is now handled by on_statusbar_scale_entered,
        // but if for some reason a coordinate pair is entered while in SVG mode,
        // we should probably just ignore it or give a specific error.
        // For now, the AppStatusBar::on_status_entry_activate handles dispatching.
        return;
    }

    if (!m_DrwArea.get_first_click_for_line()) {
        m_DrwArea.set_line_start_point(Point2D::create(x, y));
        m_DrwArea.set_line_current_mouse_pos(Point2D::create(x, y));
        m_DrwArea.set_first_click_for_line(true);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << x << "," << y;
        m_AppStatusBar.update_line_mode_status(true);
        m_AppStatusBar.get_entry().set_placeholder_text("Enter second point x,y...");
        std::cout << "Keyboard: First point set to: (" << x << ", " << y << ")" << std::endl;
    } else {
        Glib::RefPtr<Line> new_line_obj = Line::create(
            m_DrwArea.get_line_start_point(), Point2D::create(x, y),
            m_DrwArea.get_current_line_thickness(), m_DrwArea.get_current_line_style(),
            m_DrwArea.get_current_line_color_r(), m_DrwArea.get_current_line_color_g(), m_DrwArea.get_current_line_color_b()
        );

        if (m_DrwArea.get_tree_model() && m_DrwArea.get_columns_ptr() && m_DrwArea.get_tree_view_ptr()) {
            Gtk::TreeModel::iterator parent_line_it = m_DrwArea.get_tree_model()->append();
            auto row = *parent_line_it;
            row[m_DrwArea.get_columns_ptr()->m_col_id] = m_DrwArea.get_next_object_id()++;
            row[m_DrwArea.get_columns_ptr()->m_col_name] = "Line";
            row[m_DrwArea.get_columns_ptr()->m_col_object_type] = ObjectType::LINE;
            row[m_DrwArea.get_columns_ptr()->m_col_generic_object] = new_line_obj;
            row[m_DrwArea.get_columns_ptr()->m_col_thickness] = new_line_obj->thickness;
            row[m_DrwArea.get_columns_ptr()->m_col_style_name] = new_line_obj->get_style_name();
            row[m_DrwArea.get_columns_ptr()->m_col_color_name] = new_line_obj->get_color_name();


            std::stringstream ss_start_kb, ss_end_kb, ss_mid_kb;
            ss_start_kb << std::fixed << std::setprecision(2) << m_DrwArea.get_line_start_point()->x << ", " << m_DrwArea.get_line_start_point()->y;
            ss_end_kb << std::fixed << std::setprecision(2) << x << ", " << y;

            Gtk::TreeModel::iterator child_it_start = m_DrwArea.get_tree_model()->append(row.children());
            auto child_row_start = *child_it_start;
            child_row_start[m_DrwArea.get_columns_ptr()->m_col_id] = m_DrwArea.get_next_object_id()++;
            child_row_start[m_DrwArea.get_columns_ptr()->m_col_name] = "Start Point (" + ss_start_kb.str() + ")";
            child_row_start[m_DrwArea.get_columns_ptr()->m_col_object_type] = ObjectType::POINT;
            child_row_start[m_DrwArea.get_columns_ptr()->m_col_generic_object] = m_DrwArea.get_line_start_point();

            Gtk::TreeModel::iterator child_it_end = m_DrwArea.get_tree_model()->append(row.children());
            auto child_row_end = *child_it_end;
            child_row_end[m_DrwArea.get_columns_ptr()->m_col_name] = "End Point (" + ss_end_kb.str() + ")";
            child_row_end[m_DrwArea.get_columns_ptr()->m_col_object_type] = ObjectType::POINT;
            child_row_end[m_DrwArea.get_columns_ptr()->m_col_generic_object] = Point2D::create(x, y);

            double mid_x = (m_DrwArea.get_line_start_point()->x + x) / 2.0;
            double mid_y = (m_DrwArea.get_line_start_point()->y + y) / 2.0;
            Glib::RefPtr<Point2D> mid_point = Point2D::create(mid_x, mid_y);

            ss_mid_kb << std::fixed << std::setprecision(2) << mid_x << ", " << mid_y;

            Gtk::TreeModel::iterator child_it_mid = m_DrwArea.get_tree_model()->append(row.children());
            auto child_row_mid = *child_it_mid;
            child_row_mid[m_DrwArea.get_columns_ptr()->m_col_id] = m_DrwArea.get_next_object_id()++;
            child_row_mid[m_DrwArea.get_columns_ptr()->m_col_name] = "Mid Point (" + ss_mid_kb.str() + ")";
            child_row_mid[m_DrwArea.get_columns_ptr()->m_col_object_type] = ObjectType::POINT;
            child_row_mid[m_DrwArea.get_columns_ptr()->m_col_generic_object] = mid_point;

            m_DrwArea.get_tree_view_ptr()->expand_row(m_DrwArea.get_tree_model()->get_path(parent_line_it), false);
        }

        m_DrwArea.set_first_click_for_line(false);
        m_AppStatusBar.update_line_mode_status(false);
        m_AppStatusBar.get_entry().set_placeholder_text("Enter x,y coordinates...");
        std::cout << "Keyboard: Line drawn from (" << m_DrwArea.get_line_start_point()->x << ", " << m_DrwArea.get_line_start_point()->y << ") to (" << x << ", " << y << ")" << std::endl;
    }
    m_DrwArea.queue_draw();
}

void WinMain::on_statusbar_scale_entered(double scale)
{
    m_DrwArea.set_current_svg_placement_scale(scale);
    std::cout << "WinMain: SVG placement scale set to " << scale << " from status bar." << std::endl;
    m_AppStatusBar.get_entry().set_placeholder_text("Click on drawing area to place SVG..."); // Prompt to click after scale is set
}


void WinMain::on_menu_file_delete()
{
    std::cout << "Delete action started." << std::endl;
    auto selection = m_TreeList.get_selection();
    if (selection) {
        std::vector<Gtk::TreePath> selected_paths = selection->get_selected_rows();

        std::vector<Gtk::TreeModel::iterator> iterators_to_delete;
        for (const auto& path : selected_paths) {
            Gtk::TreeModel::iterator iter = m_TreeList.m_refTreeModel->get_iter(path);
            if (iter) {
                iterators_to_delete.push_back(iter);
            }
        }

        m_DrwArea.clear_selection_and_highlight();

        for (auto it = iterators_to_delete.rbegin(); it != iterators_to_delete.rend(); ++it) {
            Gtk::TreeModel::iterator iter = *it;
            ObjectType obj_type = (*iter)[m_TreeList.m_Columns.m_col_object_type];

            if (obj_type == ObjectType::LINE || obj_type == ObjectType::SVG) { // Also delete SVG objects
                m_TreeList.m_refTreeModel->erase(iter);
                std::cout << "Object record deleted from TreeList." << std::endl;
            } else if (obj_type == ObjectType::POINT) {
                // Points are children of lines/other objects. Deleting parent will delete children.
                // If a point is selected directly, we might need more complex logic to ensure
                // its parent line is not orphaned or partially deleted. For now, assume
                // points are deleted with their parent lines.
                std::cout << "Point record selected directly, usually deleted with parent line. Skipping direct point deletion." << std::endl;
            }
        }

        selection->unselect_all();
        m_DrwArea.redraw_all();
        m_AppStatusBar.update_line_mode_status(false);
    } else {
        m_AppStatusBar.update_line_mode_status(false);
        std::cout << "No record selected for deletion." << std::endl;
    }
}


void WinMain::on_button_clicked()
{
std::cout << "WinMain button clicked."  << std::endl;
}

void WinMain::on_menu_others()
{
  std::cout << "Menu item selected." << std::endl;
}

void WinMain::on_menu_choices(const Glib::VariantBase& parameter)
{
    Glib::ustring state_str = Glib::Variant<Glib::ustring>(const_cast<GVariant*>(parameter.gobj())).get();

    m_refChoice->change_state(Glib::Variant<Glib::ustring>::create(state_str));

    Glib::ustring message;
    if (state_str == "a")
        message = "Choice a was selected.";
    else if (state_str == "b")
        message = "Choice b was selected.";
    else
        message = "Unknown choice: " + state_str;

    std::cout << message << std::endl;
}

void WinMain::on_menu_choices_other(const Glib::VariantBase& parameter)
{
    int state_int = Glib::Variant<int>(const_cast<GVariant*>(parameter.gobj())).get();

    m_refChoiceOther->change_state(Glib::Variant<int>::create(state_int));

    Glib::ustring message;
    if (state_int == 1)
        message = "Choice 1 was selected.";
    else if (state_int == 2)
        message = "Choice 2 was selected.";
    else
        message = "Unknown choice: " + std::to_string(state_int);

    std::cout << message << std::endl;
}

void WinMain::on_menu_toggle()
{
  bool active = false;
  m_refToggle->get_state(active);

  active = !active;
  m_refToggle->change_state(active);

  Glib::ustring message;
  if (active)
    message = "Toggle is active.";
  else
    message = "Toggle is not active.";

  std::cout << message << std::endl;
}



class App2d : public Gtk::Application
{
protected:
  App2d();

public:
  static Glib::RefPtr<App2d> create();

protected:
  void on_startup() override;
  void on_activate() override;

private:
  void create_window();

  void on_menu_file_new_generic();
  void on_menu_file_newline();
  void on_menu_file_quit();
  void on_menu_help_about();
  WinMain *m_win = nullptr;
};




App2d::App2d()
: Gtk::Application("toolbar")
{
  Glib::set_application_name("Main Menu Example");
}

Glib::RefPtr<App2d> App2d::create()
{
  return Glib::make_refptr_for_instance(new App2d());
}

void App2d::on_startup()
{
  Gtk::Application::on_startup();

  add_action("newstandard",
    sigc::mem_fun(*this, &App2d::on_menu_file_new_generic));

  add_action("newfoo",
    sigc::mem_fun(*this, &App2d::on_menu_file_new_generic));

  add_action("newgoo",
    sigc::mem_fun(*this, &App2d::on_menu_file_new_generic));

  add_action("newline",
    sigc::mem_fun(*this, &App2d::on_menu_file_newline));

  add_action("quit", sigc::mem_fun(*this, &App2d::on_menu_file_quit));

  add_action("about", sigc::mem_fun(*this, &App2d::on_menu_help_about));

  set_accel_for_action("app.newstandard", "<Primary>n");
  set_accel_for_action("app.newline", "<Primary>l");
  set_accel_for_action("app.quit", "<Primary>q");
  set_accel_for_action("win.copy", "<Primary>c");
  set_accel_for_action("win.paste", "<Primary>v");
  set_accel_for_action("win.delete", "Delete");
}

void App2d::on_activate()
{
  create_window();
  if (m_win) {
      m_win->get_my_area().zoom_to_a4();
  }
}

void App2d::create_window()
{
  m_win = new WinMain();

  add_window(*m_win);

  m_win->set_visible(true);

  m_win->present();

}

void App2d::on_menu_file_new_generic()
{
  std::cout << "File|New menu item selected." << std::endl;
}

void App2d::on_menu_file_newline()
{
  std::cout << "New line action launched. Toggling line drawing mode." << std::endl;
  if (m_win) {
      m_win->get_my_area().toggle_line_drawing_mode();
  }
}

void App2d::on_menu_file_quit()
{
  std::cout << "File|Quit menu item selected." << std::endl;
  quit();
}

void App2d::on_menu_help_about()
{
  std::cout << "Help|About menu item selected." << std::endl;
}

const char* GETTEXT_PACKAGE = "app";
const char* PROGRAMNAME_LOCALEDIR = "locale";

int main(int argc, char* argv[])
{
   /* 
    const char* preferred_locale = "cs_CZ.UTF-8";
    if (std::setlocale(LC_ALL, preferred_locale) == NULL) {
        std::cerr << "Warning: Could not set locale to " << preferred_locale << ". Trying default." << std::endl;
        if (std::setlocale(LC_ALL, "") == NULL) {
            std::cerr << "Error: Could not set any locale. Text encoding issues may occur." << std::endl;
        }
    }
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::cerr.imbue(std::locale());

    std::string lang_setting;
    std::ifstream myfilei("appconfig.cfg");
    if (myfilei.is_open()) {
        std::getline(myfilei, lang_setting);
        myfilei.close();
        std::cout << "Preference settings for LANG from appconfig.cfg: " << lang_setting << std::endl;
    } else {
        std::cerr << "Warning: appconfig.cfg not found. Using default locale for Glib/GTK." << std::endl;
        
        lang_setting = preferred_locale;
    }

    
    Glib::setenv("LANG", lang_setting.c_str(), true);
    std::cout << "LANG environment variable after Glib::setenv: " << getenv("LANG") << std::endl;

    bindtextdomain(GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
*/
    try {
        auto app = App2d::create();
        int result = app->run(argc, argv);
        std::cout << "Application exited with code: " << result << std::endl;
        return result;
    } catch (const Glib::Error& ex) {
        std::cerr << "Glib::Error caught: " << ex.what() << std::endl;
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Standard exception caught: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught!" << std::endl;
        return 1;
    }
}
