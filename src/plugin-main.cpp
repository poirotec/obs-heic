#include <obs-frontend-api.h>
#include <obs-module.h>

#include <graphics/graphics.h>

#include <libheif/heif.h>

#include <QApplication>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QPointer>
#include <QUrl>
#include <QWidget>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-heic", "en-US")
OBS_MODULE_AUTHOR("Codex")

namespace {

struct HeicSource {
	gs_texture_t *texture = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct DecodedImage {
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> rgba;
};

static bool heif_ok(const heif_error &error, const char *operation, const char *file)
{
	if (error.code == heif_error_Ok)
		return true;
	blog(LOG_ERROR, "[obs-heic] %s failed for '%s': %s (%d/%d)", operation, file,
	     error.message ? error.message : "unknown error", static_cast<int>(error.code),
	     static_cast<int>(error.subcode));
	return false;
}

static bool decode_heic(const QString &path, DecodedImage &output)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		blog(LOG_ERROR, "[obs-heic] Could not open '%s'", path.toUtf8().constData());
		return false;
	}
	const QByteArray bytes = file.readAll();
	const QByteArray utf8_path = path.toUtf8();
	if (bytes.isEmpty()) {
		blog(LOG_ERROR, "[obs-heic] File is empty: '%s'", utf8_path.constData());
		return false;
	}

	heif_context *context = heif_context_alloc();
	if (!context)
		return false;

	heif_error error = heif_context_read_from_memory_without_copy(
		context, bytes.constData(), static_cast<size_t>(bytes.size()), nullptr);
	if (!heif_ok(error, "Reading HEIF", utf8_path.constData())) {
		heif_context_free(context);
		return false;
	}

	heif_image_handle *handle = nullptr;
	error = heif_context_get_primary_image_handle(context, &handle);
	if (!heif_ok(error, "Finding primary image", utf8_path.constData())) {
		heif_context_free(context);
		return false;
	}

	heif_decoding_options *options = heif_decoding_options_alloc();
	if (options)
		options->convert_hdr_to_8bit = true;

	heif_image *image = nullptr;
	error = heif_decode_image(handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA,
				 options);
	if (options)
		heif_decoding_options_free(options);
	heif_image_handle_release(handle);
	heif_context_free(context);
	if (!heif_ok(error, "Decoding HEIF", utf8_path.constData()))
		return false;

	const int width = heif_image_get_width(image, heif_channel_interleaved);
	const int height = heif_image_get_height(image, heif_channel_interleaved);
	int stride = 0;
	const uint8_t *pixels = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);
	if (!pixels || width <= 0 || height <= 0 || stride < width * 4) {
		blog(LOG_ERROR, "[obs-heic] Invalid decoded image for '%s'", utf8_path.constData());
		heif_image_release(image);
		return false;
	}

	output.width = static_cast<uint32_t>(width);
	output.height = static_cast<uint32_t>(height);
	output.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
	for (int y = 0; y < height; ++y) {
		std::memcpy(output.rgba.data() + static_cast<size_t>(y) * width * 4,
			    pixels + static_cast<size_t>(y) * stride, static_cast<size_t>(width) * 4);
	}
	heif_image_release(image);

	// OBS's standard image effect uses premultiplied alpha blending.
	for (size_t i = 0; i < output.rgba.size(); i += 4) {
		const unsigned int alpha = output.rgba[i + 3];
		output.rgba[i] = static_cast<uint8_t>((output.rgba[i] * alpha + 127) / 255);
		output.rgba[i + 1] = static_cast<uint8_t>((output.rgba[i + 1] * alpha + 127) / 255);
		output.rgba[i + 2] = static_cast<uint8_t>((output.rgba[i + 2] * alpha + 127) / 255);
	}
	return true;
}

static void destroy_texture(HeicSource *source)
{
	if (!source->texture)
		return;
	obs_enter_graphics();
	gs_texture_destroy(source->texture);
	obs_leave_graphics();
	source->texture = nullptr;
	source->width = 0;
	source->height = 0;
}

static void update_source(void *data, obs_data_t *settings)
{
	auto *source = static_cast<HeicSource *>(data);
	destroy_texture(source);

	const char *file_setting = obs_data_get_string(settings, "file");
	if (!file_setting || !*file_setting)
		return;

	DecodedImage decoded;
	if (!decode_heic(QString::fromUtf8(file_setting), decoded))
		return;

	const uint8_t *plane = decoded.rgba.data();
	obs_enter_graphics();
	source->texture = gs_texture_create(decoded.width, decoded.height, GS_RGBA, 1, &plane, 0);
	obs_leave_graphics();
	if (!source->texture) {
		blog(LOG_ERROR, "[obs-heic] Could not create texture for '%s'", file_setting);
		return;
	}
	source->width = decoded.width;
	source->height = decoded.height;
	blog(LOG_INFO, "[obs-heic] Decoded '%s' at %ux%u", file_setting, decoded.width, decoded.height);
}

static const char *source_name(void *)
{
	return obs_module_text("HEICSource");
}

static void *create_source(obs_data_t *settings, obs_source_t *)
{
	auto *source = new HeicSource;
	update_source(source, settings);
	return source;
}

static void destroy_source(void *data)
{
	auto *source = static_cast<HeicSource *>(data);
	destroy_texture(source);
	delete source;
}

static uint32_t source_width(void *data)
{
	return static_cast<HeicSource *>(data)->width;
}

static uint32_t source_height(void *data)
{
	return static_cast<HeicSource *>(data)->height;
}

static void render_source(void *data, gs_effect_t *effect)
{
	auto *source = static_cast<HeicSource *>(data);
	if (!source->texture)
		return;

	const bool previous_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(image, source->texture);
	gs_draw_sprite(source->texture, 0, source->width, source->height);
	gs_blend_state_pop();
	gs_enable_framebuffer_srgb(previous_srgb);
}

static obs_properties_t *source_properties(void *)
{
	obs_properties_t *properties = obs_properties_create();
	obs_properties_add_path(properties, "file", obs_module_text("File"), OBS_PATH_FILE,
				obs_module_text("FileFilter"), nullptr);
	return properties;
}

static obs_source_info source_info = [] {
	obs_source_info info = {};
	info.id = "heic_source";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB;
	info.get_name = source_name;
	info.create = create_source;
	info.destroy = destroy_source;
	info.update = update_source;
	info.get_width = source_width;
	info.get_height = source_height;
	info.video_render = render_source;
	info.get_properties = source_properties;
	info.icon_type = OBS_ICON_TYPE_IMAGE;
	return info;
}();

static std::string unique_source_name(const QString &base)
{
	std::string stem = base.toUtf8().constData();
	for (unsigned int number = 1;; ++number) {
		std::string candidate = stem;
		if (number > 1)
			candidate += " (" + std::to_string(number) + ")";
		obs_source_t *existing = obs_get_source_by_name(candidate.c_str());
		if (!existing)
			return candidate;
		obs_source_release(existing);
	}
}

static bool is_heif_file(const QString &path)
{
	const QString suffix = QFileInfo(path).suffix();
	return suffix.compare("heic", Qt::CaseInsensitive) == 0 ||
	       suffix.compare("heif", Qt::CaseInsensitive) == 0 ||
	       suffix.compare("hif", Qt::CaseInsensitive) == 0;
}

static void add_dropped_source(const QString &path)
{
	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source)
		return;
	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (!scene) {
		obs_source_release(scene_source);
		return;
	}

	const QFileInfo file_info(path);
	const QByteArray utf8_path = file_info.absoluteFilePath().toUtf8();
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "file", utf8_path.constData());
	const std::string name = unique_source_name(file_info.fileName());
	obs_source_t *source = obs_source_create("heic_source", name.c_str(), settings, nullptr);
	obs_data_release(settings);
	if (source) {
		obs_scene_add(scene, source);
		obs_source_release(source);
	}
	obs_source_release(scene_source);
}

class DropFilter final : public QObject {
public:
	using QObject::QObject;

protected:
	bool eventFilter(QObject *watched, QEvent *event) override
	{
		if (event->type() != QEvent::Drop)
			return QObject::eventFilter(watched, event);
		auto *widget = qobject_cast<QWidget *>(watched);
		auto *main_window = static_cast<QWidget *>(obs_frontend_get_main_window());
		if (!widget || !main_window || widget->window() != main_window)
			return QObject::eventFilter(watched, event);
		auto *drop = static_cast<QDropEvent *>(event);
		if (!drop->mimeData()->hasUrls())
			return QObject::eventFilter(watched, event);
		for (const QUrl &url : drop->mimeData()->urls()) {
			const QString path = url.toLocalFile();
			if (!path.isEmpty() && QFileInfo::exists(path) && is_heif_file(path))
				add_dropped_source(path);
		}
		return QObject::eventFilter(watched, event);
	}
};

static QPointer<DropFilter> drop_filter;

} // namespace

bool obs_module_load(void)
{
	if (!qApp)
		return false;
	heif_init(nullptr);
	obs_register_source(&source_info);
	drop_filter = new DropFilter;
	qApp->installEventFilter(drop_filter);
	blog(LOG_INFO, "[obs-heic] Plugin loaded (version 1.1.0, libheif %s)", heif_get_version());
	return true;
}

void obs_module_unload(void)
{
	if (drop_filter && qApp) {
		qApp->removeEventFilter(drop_filter);
		delete drop_filter;
		drop_filter = nullptr;
	}
	heif_deinit();
}
