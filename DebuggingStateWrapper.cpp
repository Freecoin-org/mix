/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file DebuggingState.h
 * @author Yann yann@ethdev.com
 * @date 2014
 * Used to translate c++ type (u256, bytes, ...) into friendly value (to be used by QML).
 */

#include <QDebug>
#include <QString>
#include <QTextStream>
#include "libdevcrypto/Common.h"
#include "libevmcore/Instruction.h"
#include "libdevcore/Common.h"
#include "DebuggingStateWrapper.h"
using namespace dev;
using namespace dev::eth;
using namespace dev::mix;

std::tuple<QList<QObject*>, QQMLMap*> DebuggingStateWrapper::getHumanReadableCode(const bytes& code)
{
	QList<QObject*> codeStr;
	QMap<int, int> codeMapping;
	for (unsigned i = 0; i <= code.size(); ++i)
	{
		byte b = i < code.size() ? code[i] : 0;
		try
		{
			QString s = QString::fromStdString(instructionInfo((Instruction)b).name);
			std::ostringstream out;
			out << hex << std::setw(4) << std::setfill('0') << i;
			codeMapping[i] = codeStr.size();
			int line = i;
			if (b >= (byte)Instruction::PUSH1 && b <= (byte)Instruction::PUSH32)
			{
				unsigned bc = b - (byte)Instruction::PUSH1 + 1;
				s = "PUSH 0x" + QString::fromStdString(toHex(bytesConstRef(&code[i + 1], bc)));
				i += bc;
			}
			HumanReadableCode* humanCode = new HumanReadableCode(QString::fromStdString(out.str()) + "  "  + s, line);
			codeStr.append(humanCode);
		}
		catch (...)
		{
			qDebug() << QString("Unhandled exception!") << endl <<
					 QString::fromStdString(boost::current_exception_diagnostic_information());
			break;	// probably hit data segment
		}
	}
	return std::make_tuple(codeStr, new QQMLMap(codeMapping));
}

QString DebuggingStateWrapper::debugStack()
{
	QString stack;
	for (auto i: m_state.stack)
		stack.prepend(prettyU256(i) + "\n");

	return stack;
}

QString DebuggingStateWrapper::debugStorage()
{
	std::stringstream s;
	for (auto const& i: m_state.storage)
		s << "@" << prettyU256(i.first).toStdString() << "&nbsp;&nbsp;&nbsp;&nbsp;" << prettyU256(i.second).toStdString();

	return QString::fromStdString(s.str());
}

QString DebuggingStateWrapper::debugMemory()
{
	return QString::fromStdString(memDump(m_state.memory, 16, false));
}

QString DebuggingStateWrapper::debugCallData()
{

	return QString::fromStdString(memDump(m_data, 16, false));
}

QStringList DebuggingStateWrapper::levels()
{
	QStringList levelsStr;
	for (unsigned i = 0; i <= m_state.levels.size(); ++i)
	{
		DebuggingState const& s = i ? *m_state.levels[m_state.levels.size() - i] : m_state;
		std::ostringstream out;
		out << m_state.cur.abridged();
		if (i)
			out << " " << instructionInfo(m_state.inst).name << " @0x" << hex << m_state.curPC;
		levelsStr.append(QString::fromStdString(out.str()));
	}
	return levelsStr;
}

QString DebuggingStateWrapper::headerInfo()
{
	std::ostringstream ss;
	ss << dec << " STEP: " << m_state.steps << "  |  PC: 0x" << hex << m_state.curPC << "  :  " << dev::eth::instructionInfo(m_state.inst).name << "  |  ADDMEM: " << dec << m_state.newMemSize << " words  |  COST: " << dec << m_state.gasCost <<  "  |  GAS: " << dec << m_state.gas;
	return QString::fromStdString(ss.str());
}

QString DebuggingStateWrapper::endOfDebug()
{
	if (m_state.gasCost > m_state.gas)
		return "OUT-OF-GAS";
	else if (m_state.inst == Instruction::RETURN && m_state.stack.size() >= 2)
	{
		unsigned from = (unsigned)m_state.stack.back();
		unsigned size = (unsigned)m_state.stack[m_state.stack.size() - 2];
		unsigned o = 0;
		bytes out(size, 0);
		for (; o < size && from + o < m_state.memory.size(); ++o)
			out[o] = m_state.memory[from + o];
		return "RETURN " + QString::fromStdString(dev::memDump(out, 16, false));
	}
	else if (m_state.inst == Instruction::STOP)
		 return "STOP";
	else if (m_state.inst == Instruction::SUICIDE && m_state.stack.size() >= 1)
		 return "SUICIDE 0x" + QString::fromStdString(toString(right160(m_state.stack.back())));
	else
		return "EXCEPTION";
}

QString DebuggingStateWrapper::prettyU256(u256 _n)
{
	unsigned inc = 0;
	QString raw;
	std::ostringstream s;
	if (!(_n >> 64))
		s << " " << (uint64_t)_n << " (0x" << hex << (uint64_t)_n << ")";
	else if (!~(_n >> 64))
		s << " " << (int64_t)_n << " (0x" << hex << (int64_t)_n << ")";
	else if ((_n >> 160) == 0)
	{
		Address a = right160(_n);

		QString n = QString::fromStdString(a.abridged());//pretty(a);
		if (n.isNull())
			s << "0x" << a;
		else
			s << n.toHtmlEscaped().toStdString() << "(0x" << a.abridged() << ")";
	}
	else if ((raw = fromRaw((h256)_n, &inc)).size())
		return "\"" + raw.toHtmlEscaped() + "\"" + (inc ? " + " + QString::number(inc) : "");
	else
		s << "" << (h256)_n;
	return QString::fromStdString(s.str());
}

QString DebuggingStateWrapper::fromRaw(h256 _n, unsigned* _inc)
{
	if (_n)
	{
		std::string s((char const*)_n.data(), 32);
		auto l = s.find_first_of('\0');
		if (!l)
			return QString();
		if (l != std::string::npos)
		{
			auto p = s.find_first_not_of('\0', l);
			if (!(p == std::string::npos || (_inc && p == 31)))
				return QString();
			if (_inc)
				*_inc = (byte)s[31];
			s.resize(l);
		}
		for (auto i: s)
			if (i < 32)
				return QString();
		return QString::fromStdString(s);
	}
	return QString();
}
