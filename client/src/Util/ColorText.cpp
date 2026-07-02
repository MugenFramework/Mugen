#include <Util/ColorText.h>

QString MugenNamespace::Util::ColorText::Colors::Hex::Background    = "#0a0a0f";
QString MugenNamespace::Util::ColorText::Colors::Hex::Foreground    = "#f0f0ee";
QString MugenNamespace::Util::ColorText::Colors::Hex::Comment       = "#8888aa";
QString MugenNamespace::Util::ColorText::Colors::Hex::CurrentLine   = "#1c1c28";

QString MugenNamespace::Util::ColorText::Colors::Hex::Cyan          = "#7dd3fc";
QString MugenNamespace::Util::ColorText::Colors::Hex::Green         = "#50fa7b";
QString MugenNamespace::Util::ColorText::Colors::Hex::Orange        = "#ffb86c";
QString MugenNamespace::Util::ColorText::Colors::Hex::Pink          = "#ff6b9d";
QString MugenNamespace::Util::ColorText::Colors::Hex::Purple        = "#c084fc";
QString MugenNamespace::Util::ColorText::Colors::Hex::Red           = "#ff5555";
QString MugenNamespace::Util::ColorText::Colors::Hex::Yellow        = "#fde68a";

QString MugenNamespace::Util::ColorText::Colors::Hex::SessionCyan   = "#1a3a4a";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionGreen  = "#1a3a1a";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionOrange = "#3a2a10";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionPink   = "#3a1a2a";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionPurple = "#2a1a3a";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionRed    = "#3a1a1a";
QString MugenNamespace::Util::ColorText::Colors::Hex::SessionYellow = "#3a3010";

void MugenNamespace::Util::ColorText::SetDraculaDark()
{
    MugenNamespace::Util::ColorText::Colors::Hex::Background    = "#282a36";
    MugenNamespace::Util::ColorText::Colors::Hex::Foreground    = "#f8f8f2";
    MugenNamespace::Util::ColorText::Colors::Hex::Comment       = "#6272a4";
    MugenNamespace::Util::ColorText::Colors::Hex::CurrentLine   = "#44475a";

    MugenNamespace::Util::ColorText::Colors::Hex::Cyan          = "#8be9fd";
    MugenNamespace::Util::ColorText::Colors::Hex::Green         = "#50fa7b";
    MugenNamespace::Util::ColorText::Colors::Hex::Orange        = "#ffb86c";
    MugenNamespace::Util::ColorText::Colors::Hex::Pink          = "#ff79c6";
    MugenNamespace::Util::ColorText::Colors::Hex::Purple        = "#bd93f9";
    MugenNamespace::Util::ColorText::Colors::Hex::Red           = "#ff5555";
    MugenNamespace::Util::ColorText::Colors::Hex::Yellow        = "#f1fa8c";
}

void MugenNamespace::Util::ColorText::SetDraculaLight()
{
    // TODO: get white theme
}

QString MugenNamespace::Util::ColorText::Color(const QString& color, const QString &text)
{
    return "<span style=\"color: "+ color +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Background(const QString& text)
{
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Background +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Foreground(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Foreground +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Comment(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Comment +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Cyan(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Cyan +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Green(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Green +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Orange(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Orange +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Pink(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Pink +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Purple(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Purple +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Red(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Red +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Yellow(const QString& text) {
    return "<span style=\"color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Yellow +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::Bold(const QString& text) {
    return "<b>" + text.toHtmlEscaped() + "</b>";
}

QString MugenNamespace::Util::ColorText::Underline(const QString &text) {
    return "<span style=\"text-decoration:underline\">" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineBackground(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Background +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineForeground(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Foreground +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineComment(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Comment +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineCyan(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Cyan +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineGreen(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Green +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineOrange(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Orange +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlinePink(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Pink +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlinePurple(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Purple +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineRed(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Red +";\" >" + text.toHtmlEscaped() + "</span>";
}

QString MugenNamespace::Util::ColorText::UnderlineYellow(const QString &text) {
    return "<span style=\"text-decoration:underline; color: "+ MugenNamespace::Util::ColorText::Colors::Hex::Yellow +";\" >" + text.toHtmlEscaped() + "</span>";
}
