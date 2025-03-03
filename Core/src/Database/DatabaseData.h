#pragma once

namespace Core
{
	enum class DatabaseDataType
	{
		None = 0,
		Int,
		String,
		Bool,
		Enum,
		Timestamp,
	};

	struct DatabaseData
	{
		virtual inline const DatabaseDataType GetType() const { return DatabaseDataType::None; }
		virtual inline void* GetValue() { return nullptr; }
		inline const char* GetValueCharPtr() { return (const char*)GetValue(); }

		template<typename T>
		inline T GetValue() { return *(T*)GetValue(); }

		virtual void Serialize(std::ostream& os) = 0;
		virtual void Deserialize(std::istream& is) = 0;
	};

	struct DatabaseInt : public DatabaseData
	{
		DatabaseInt() = default;
		DatabaseInt(int value) : Value(value) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Int; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Value; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Value), sizeof(Value)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Value), sizeof(Value)); }

		int Value;
	};

	struct DatabaseString : public DatabaseData
	{
		DatabaseString() = default;
		DatabaseString(const char* string)
		{
			strncpy(String, string, sizeof(String) - 1);
			String[sizeof(String) - 1] = '\0';
		}

		DatabaseString(std::string& string)
		{
			strncpy(String, string.c_str(), string.size());
			String[string.size()] = '\0';
		}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::String; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &String; }

		void Serialize(std::ostream& os) override { os.write(String, sizeof(String)); }
		void Deserialize(std::istream& is) override { is.read(String, sizeof(String)); }

		char String[256];
	};

	struct DatabaseBool : public DatabaseData
	{
		DatabaseBool() = default;
		DatabaseBool(bool value) : Value(value) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Bool; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Value; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Value), sizeof(Value)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Value), sizeof(Value)); }

		bool Value;
	};

	struct DatabaseTimestamp : public DatabaseData
	{
		DatabaseTimestamp() = default;
		DatabaseTimestamp(time_t time) : Time(time) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Timestamp; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Time; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Time), sizeof(Time)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Time), sizeof(Time)); }
		
		time_t Time;
	};
}