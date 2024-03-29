#include "sylar/log.h"
#include "sylar/util.h"
#include "sylar/singleton.h"
#include <thread>

int main(int argc, char** argv)
{
	sylar::Logger::ptr logger(new sylar::Logger);
	logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

	sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
	sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%m%n"));
	file_appender->setFormatter(fmt);
	file_appender->setLevel(sylar::LogLevel::ERROR);

	logger->addAppender(file_appender);
	// sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0,
	//   sylar::getThreadId(), sylar::getFiberId(), time(0)));
	//logger->log(sylar::LogLevel::DEBUG, event);
	//std::cout << event->getContent();
	SYLAR_LOG_INFO(logger) << "test macro";
	SYLAR_LOG_ERROR(logger) << "test macro error";

	SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");


	auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
	SYLAR_LOG_INFO(l) << "xxx";
	return 0;
}