#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include "effects_base_ui.hpp"

EffectsBaseUi::EffectsBaseUi(BaseObjectType* cobject,
                             const Glib::RefPtr<Gtk::Builder>& refBuilder,
                             const Glib::RefPtr<Gio::Settings>& refSettings,
                             const std::shared_ptr<PulseManager>& pulse_manager)
    : Gtk::Box(cobject),
      settings(refSettings),
      builder(refBuilder),
      pm(pulse_manager) {
    // loading glade widgets

    builder->get_widget("stack", stack);
    builder->get_widget("listbox", listbox);
    builder->get_widget("apps_box", apps_box);

    auto row = new Gtk::ListBoxRow();

    row->set_name("applications");
    row->set_margin_top(6);
    row->set_margin_bottom(6);

    auto row_label = new Gtk::Label(std::string("<b>") + _("Applications") +
                                    std::string("</b>"));

    row_label->set_halign(Gtk::Align::ALIGN_START);
    row_label->set_use_markup(true);

    row->add(*row_label);

    listbox->add(*row);

    listbox->signal_row_activated().connect(
        [&](auto row) { stack->set_visible_child(row->get_name()); });

    listbox->set_sort_func(
        sigc::mem_fun(*this, &EffectsBaseUi::on_listbox_sort));

    connections.push_back(settings->signal_changed("plugins").connect(
        [=](auto key) { listbox->invalidate_sort(); }));

    // checking if plugin list is missing any plugin

    auto plugins = Glib::Variant<std::vector<std::string>>();
    auto default_plugins = Glib::Variant<std::vector<std::string>>();

    settings->get_value("plugins", plugins);
    settings->get_default_value("plugins", default_plugins);

    if (plugins.get().size() != default_plugins.get().size()) {
        settings->reset("plugins");
    }
}

EffectsBaseUi::~EffectsBaseUi() {}

void EffectsBaseUi::on_app_added(std::shared_ptr<AppInfo> app_info) {
    auto appui = AppInfoUi::create(app_info, pm);

    apps_box->add(*appui);

    apps_list.push_back(move(appui));
}

void EffectsBaseUi::on_app_changed(std::shared_ptr<AppInfo> app_info) {
    for (auto it = apps_list.begin(); it != apps_list.end(); it++) {
        auto n = it - apps_list.begin();

        if (apps_list[n]->app_info->index == app_info->index) {
            apps_list[n]->update(app_info);
        }
    }
}

void EffectsBaseUi::on_app_removed(uint idx) {
    for (auto it = apps_list.begin(); it != apps_list.end(); it++) {
        auto n = it - apps_list.begin();

        if (apps_list[n]->app_info->index == idx) {
            auto appui = move(apps_list[n]);

            apps_box->remove(*appui);

            apps_list.erase(it);

            break;
        }
    }
}

int EffectsBaseUi::on_listbox_sort(Gtk::ListBoxRow* row1,
                                   Gtk::ListBoxRow* row2) {
    auto name1 = row1->get_name();
    auto name2 = row2->get_name();

    auto order = Glib::Variant<std::vector<std::string>>();

    settings->get_value("plugins", order);

    auto vorder = order.get();

    auto r1 = std::find(std::begin(vorder), std::end(vorder), name1);
    auto r2 = std::find(std::begin(vorder), std::end(vorder), name2);

    auto idx1 = r1 - vorder.begin();
    auto idx2 = r2 - vorder.begin();

    // we do not want the applications row to be moved

    if (name1 == std::string("applications")) {
        return -1;
    }

    if (name2 == std::string("applications")) {
        return 1;
    }

    if (idx1 < idx2) {
        return -1;
    } else if (idx1 > idx2) {
        return 1;
    } else {
        return 0;
    }
}