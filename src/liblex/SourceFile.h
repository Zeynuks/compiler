#ifndef LEX_SOURCEFILE_H
#define LEX_SOURCEFILE_H

#include <filesystem>
#include <string>

namespace lex
{

/*
 * Загружает исходный текст программы из файла в память без изменений.
 * Бросает std::runtime_error, если файл не удалось прочитать.
 */
std::string ReadSourceFile(const std::filesystem::path& path);

} // namespace lex

#endif // LEX_SOURCEFILE_H
