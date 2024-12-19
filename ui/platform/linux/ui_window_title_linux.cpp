// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "ui/platform/linux/ui_window_title_linux.h"

#include "base/platform/linux/base_linux_xdp_utilities.h"

#include "base/integration.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xsettings.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

namespace Ui {
namespace Platform {
namespace internal {

TitleControls::Layout TitleControlsLayout() {
	[[maybe_unused]] static const auto Inited = [] {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
		using base::Platform::XCB::XSettings;
		if (const auto xSettings = XSettings::Instance()) {
			static const auto lifetime
				= xSettings->registerCallbackForProperty(
					"Gtk/DecorationLayout",
					[](
							xcb_connection_t *,
							const QByteArray &,
							const QVariant &) {
						NotifyTitleControlsLayoutChanged();
					});
		}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

		namespace XDP = base::Platform::XDP;
		static const XDP::SettingWatcher settingWatcher(
			"org.gnome.desktop.wm.preferences",
			"button-layout",
			[] {
				base::Integration::Instance().enterFromEventLoop([] {
					NotifyTitleControlsLayoutChanged();
				});
			});

		return true;
	}();

	const auto convert = [](const QString &keywords) {
		const auto toControl = [](const QString &keyword) {
			if (keyword == qstr("minimize")) {
				return TitleControls::Control::Minimize;
			} else if (keyword == qstr("maximize")) {
				return TitleControls::Control::Maximize;
			} else if (keyword == qstr("close")) {
				return TitleControls::Control::Close;
			}
			return TitleControls::Control::Unknown;
		};

		TitleControls::Layout result;
		const auto splitted = keywords.split(':');

		ranges::transform(
			splitted[0].split(','),
			ranges::back_inserter(result.left),
			toControl);

		if (splitted.size() > 1) {
			ranges::transform(
				splitted[1].split(','),
				ranges::back_inserter(result.right),
				toControl);
		}

		return result;
	};

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	const auto xSettingsResult = [&]()
	-> std::optional<TitleControls::Layout> {
		using base::Platform::XCB::XSettings;
		const auto xSettings = XSettings::Instance();
		if (!xSettings) {
			return std::nullopt;
		}

		const auto decorationLayout = xSettings->setting(
			"Gtk/DecorationLayout");

		if (!decorationLayout.isValid()) {
			return std::nullopt;
		}

		return convert(decorationLayout.toString());
	}();

	if (xSettingsResult.has_value()) {
		return *xSettingsResult;
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

	const auto portalResult = [&]() -> std::optional<TitleControls::Layout> {
		auto decorationLayout = base::Platform::XDP::ReadSetting(
			"org.gnome.desktop.wm.preferences",
			"button-layout");

		if (!decorationLayout.has_value()) {
			return std::nullopt;
		}

		return convert(
			QString::fromStdString(decorationLayout->get_string(nullptr)));
	}();

	if (portalResult.has_value()) {
		return *portalResult;
	}

	return TitleControls::Layout{
		.right = {
			TitleControls::Control::Minimize,
			TitleControls::Control::Maximize,
			TitleControls::Control::Close,
		}
	};
}

} // namespace internal
} // namespace Platform
} // namespace Ui
